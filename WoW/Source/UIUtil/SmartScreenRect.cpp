#include "SmartScreenRect.h"
#include <new>
#include <typeinfo>
#include "Engine/Source/Base/Coordinate.h"
#include "Engine/Source/Frame/CLayoutFrame.h"
#include "Engine/Source/Frame/CSimpleFrame.h"
#include "Engine/Source/Storm/H/stpl.h"
#include "Engine/Source/Storm/H/storm.h"
#include "Engine/Source/Tempest/C2Vector.h"
#include "Engine/Source/Tempest/C3Vector.h"
#include "Engine/Source/Tempest/CRect.h"
#include <cmath>

enum SCREENRECT_CLAMP_FLAGS : uint32_t {
    SRECTCLAMP_NONE = 0,
    SRECTCLAMP_LEFT = 1,
    SRECTCLAMP_RIGHT = 2,
    SRECTCLAMP_TOP = 4,
    SRECTCLAMP_BOTTOM = 8,
    SRECTCLAMP_HORIZONTAL = 3,
    SRECTCLAMP_VERTICAL = 12,
};

struct BFSNODE : public TSLinkedNode<BFSNODE> {
    /* 0008 */ CRect m_rect;
    /* 0018 */ int32_t m_direction;

    BFSNODE();
};

static CRect SmartScreenRectMoveUp(SCREENRECTGRIDS grid, CRect rect);
static CRect SmartScreenRectMoveLeft(SCREENRECTGRIDS grid, CRect rect);
static CRect SmartScreenRectMoveRight(SCREENRECTGRIDS grid, CRect rect);
static CRect SmartScreenRectMoveDown(SCREENRECTGRIDS grid, CRect rect);

static const float TOPBORDER = 0.0375f;
static const float BOTTOMBORDER = 0.01875f;

static const float LEFTBORDER = 0.0f;

// STATIC: 0x00A23C08
static TEST_DIRECTION s_searchDirections[9][NUM_TESTDIRECTIONS] = {
    { TEST_UP, TEST_RIGHT, TEST_DOWN, TEST_LEFT },
    { TEST_LEFT, TEST_UP, TEST_RIGHT, TEST_UP },
    { TEST_LEFT, TEST_DOWN, TEST_RIGHT, TEST_DOWN },
    { TEST_UP, TEST_LEFT, TEST_DOWN, TEST_LEFT },
    { TEST_UP, TEST_RIGHT, TEST_DOWN, TEST_RIGHT },
    { TEST_UP, TEST_LEFT, TEST_UP, TEST_LEFT },
    { TEST_UP, TEST_RIGHT, TEST_UP, TEST_RIGHT },
    { TEST_DOWN, TEST_LEFT, TEST_DOWN, TEST_LEFT },
    { TEST_DOWN, TEST_RIGHT, TEST_DOWN, TEST_RIGHT },
};

// STATIC: 0x00A23C98
static CRect (*s_moveFunctions[NUM_TESTDIRECTIONS])(SCREENRECTGRIDS grid, CRect rect) = {
    SmartScreenRectMoveUp,
    SmartScreenRectMoveLeft,
    SmartScreenRectMoveRight,
    SmartScreenRectMoveDown,
};

// STATIC: 0x00AD27B4
static TSList<BFSNODE> s_searchQueue;

// STATIC: 0x00AD27C0
static TSList<BFSNODE> s_freeNodes;

// 3.3.5 keeps three slots, but bulk clear/shutdown process only two.
static const uint32_t SCREENRECTGRID_STORAGE_COUNT = 3;
static const uint32_t SCREENRECTGRID_CLEAR_COUNT = 2;

// STATIC: 0x00C25CAC
static TSGrowableArray<CRect> s_grids[SCREENRECTGRID_STORAGE_COUNT];

// INLINED
inline BFSNODE::BFSNODE() {
    this->m_rect.minY = 0.0f;
    this->m_rect.minX = 0.0f;
    this->m_rect.maxY = 0.0f;
    this->m_rect.maxX = 0.0f;
}

// VERIFY: f32025e7 auto
// OFFSET: 0x00615050
static uint32_t SmartScreenRectClampToScreen(CRect* out, CRect rect, int32_t testOnly) {
    float screenWidth = NDCToDDCWidth(1.0f);
    float top = NDCToDDCHeight(1.0f) - TOPBORDER;

    static float s_screenWidth = screenWidth;
    static float s_bottom = top - BOTTOMBORDER;

    uint32_t flags = SRECTCLAMP_NONE;
    float width = rect.maxX - rect.minX;
    float height = rect.minY - rect.maxY;

    if (rect.minX < LEFTBORDER) {
        flags = SRECTCLAMP_LEFT;
    }

    if (screenWidth < rect.maxX) {
        flags |= SRECTCLAMP_RIGHT;
    }

    if (rect.maxY < BOTTOMBORDER) {
        flags |= SRECTCLAMP_BOTTOM;
    }

    if (rect.minY > top) {
        flags |= SRECTCLAMP_TOP;
    }

    if (testOnly) {
        return flags;
    }

    if (!flags) {
        *out = rect;
        return 0;
    }

    if (flags & SRECTCLAMP_VERTICAL) {
        if (flags & SRECTCLAMP_TOP) {
            rect.minY = top;
            rect.maxY = top - height;
        } else if (flags & SRECTCLAMP_BOTTOM) {
            rect.maxY = BOTTOMBORDER;
            rect.minY = height + BOTTOMBORDER;
        }
    }

    if (flags & SRECTCLAMP_HORIZONTAL) {
        if (flags & SRECTCLAMP_LEFT) {
            rect.minX = LEFTBORDER;
            rect.maxX = width;
        } else if (flags & SRECTCLAMP_RIGHT) {
            rect.maxX = screenWidth;
            rect.minX = screenWidth - width;
        }
    }

    *out = rect;

    return flags;
}

// VERIFY: 986986ac manual
// OFFSET: 0x006151C0
static uint32_t SmartScreenRectGridPos(const CRect& rect) {
    float screenWidth = NDCToDDCWidth(1.0f);
    float screenHeight = NDCToDDCHeight(1.0f);

    uint32_t columns = static_cast<uint32_t>(screenWidth / (rect.maxX - rect.minX)) + 1;
    int32_t rows = static_cast<int32_t>(screenHeight / (rect.minY - rect.maxY)) + 1;

    return columns * rows;
}

// VERIFY: a552601f manual
// OFFSET: 0x00615240
static void SmartScreenRectClipRect(CRect& rect) {
    float height = rect.minY - rect.maxY;
    float width = rect.maxX - rect.minX;
    float screenWidth = NDCToDDCWidth(1.0f);
    float screenHeight = NDCToDDCHeight(1.0f);

    if (screenHeight <= rect.minY) {
        rect.minY = screenHeight;
        rect.maxY = screenHeight - height;
    }

    if (rect.maxY < 0.0f) {
        rect.maxY = 0.0f;
        rect.minY = height;
    }

    if (rect.minX < LEFTBORDER) {
        rect.minX = LEFTBORDER;
        rect.maxX = width;
    }

    if (screenWidth < rect.maxX) {
        rect.maxX = screenWidth;
        rect.minX = screenWidth - width;
    }
}

// VERIFY: d59ec448 manual
// OFFSET: 0x00615380
static int32_t SmartScreenRectFindOverlap(SCREENRECTGRIDS grid, const CRect& rect, uint32_t direction, float* distance) {
    TSGrowableArray<CRect>& rects = s_grids[grid];

    for (uint32_t i = 0; i < rects.Count(); i++) {
        const CRect& other = rects.m_data[i];

        if (!(other.minX < rect.maxX && other.maxX > rect.minX && other.maxY < rect.minY && other.minY > rect.maxY)) {
            continue;
        }

        switch (direction) {
            case TEST_UP:
                *distance = other.minY - rect.maxY;
                return 1;

            case TEST_LEFT:
                *distance = rect.maxX - other.minX;
                return 1;

            case TEST_RIGHT:
                *distance = other.maxX - rect.minX;
                return 1;

            case TEST_DOWN:
                *distance = rect.minY - other.maxY;
                return 1;
        }
    }

    return 0;
}

// VERIFY: d7e69ac5 auto
// OFFSET: 0x00615430
static CRect SmartScreenRectMoveUp(SCREENRECTGRIDS grid, CRect rect) {
    float distance;

    if (SmartScreenRectFindOverlap(grid, rect, TEST_UP, &distance)) {
        rect.minY = rect.minY + distance;
        rect.maxY = distance + rect.maxY;
    }

    return rect;
}

// VERIFY: 9eabe4b2 manual
// OFFSET: 0x00615480
static CRect SmartScreenRectMoveLeft(SCREENRECTGRIDS grid, CRect rect) {
    float distance;

    if (SmartScreenRectFindOverlap(grid, rect, TEST_LEFT, &distance)) {
        rect.minX = rect.minX - distance;
        rect.maxX = rect.maxX - distance;
    }

    return rect;
}

// VERIFY: c917b611 manual
// OFFSET: 0x00615520
static CRect SmartScreenRectMoveRight(SCREENRECTGRIDS grid, CRect rect) {
    float distance;

    if (SmartScreenRectFindOverlap(grid, rect, TEST_RIGHT, &distance)) {
        rect.minX = rect.minX + distance;
        rect.maxX = rect.maxX + distance;
    }

    return rect;
}

// VERIFY: 9b0e0ff0 manual
// OFFSET: 0x006155C0
static CRect SmartScreenRectMoveDown(SCREENRECTGRIDS grid, CRect rect) {
    float distance;

    if (SmartScreenRectFindOverlap(grid, rect, TEST_DOWN, &distance)) {
        rect.minY = rect.minY - distance;
        rect.maxY = rect.maxY - distance;
    }

    return rect;
}

// VERIFY: 5a10abbf auto
// OFFSET: 0x006157C0
static void SmartScreenRectRecycleNodes() {
    s_freeNodes.Combine(&s_searchQueue, 2, nullptr);
}

// VERIFY: 642ef51a auto
// OFFSET: 0x00615860
void SmartScreenRectClearAllGrids() {
    for (uint32_t i = 0; i < SCREENRECTGRID_CLEAR_COUNT; i++) {
        s_grids[i].SetCount(0);
    }
}

// VERIFY: a5840a2c auto
// OFFSET: 0x00615890
void SmartScreenRectClearGrid(SCREENRECTGRIDS grid) {
    if (grid <= SRECTGRID_UNKNOWN_2) {
        s_grids[grid].SetCount(0);
    }
}

// VERIFY: dcfdc3d7 manual
// OFFSET: 0x006158C0
static BFSNODE* SmartScreenRectAllocNode() {
    BFSNODE* node = s_freeNodes.Head();

    if (node) {
        s_searchQueue.LinkToHead(node);
        node->m_direction = TEST_INVALID;
        return node;
    }

    node = s_searchQueue.NewNode(2, 0, 0);
    node->m_direction = TEST_INVALID;

    return node;
}

// VERIFY: a1ae77d0 manual
// OFFSET: 0x00615980
static CRect SmartScreenRectFindFreeRect(SCREENRECTGRIDS grid, const CRect& rect) {
    CRect result;
    result.minY = 0.0f;
    result.minX = 0.0f;
    result.maxY = 0.0f;
    result.maxX = 0.0f;

    uint32_t flags = SmartScreenRectClampToScreen(&result, rect, 0);
    int32_t start;

    if (flags & SRECTCLAMP_LEFT) {
        start = (flags & SRECTCLAMP_TOP) ? 8 : ((flags & SRECTCLAMP_BOTTOM) | 0x10) >> 2;
    } else if (flags & SRECTCLAMP_TOP) {
        start = (flags & SRECTCLAMP_RIGHT) ? 7 : 2;
    } else if (flags & SRECTCLAMP_RIGHT) {
        start = (flags & SRECTCLAMP_BOTTOM) ? 5 : 3;
    } else {
        start = (flags >> 3) & 1;
    }

    BFSNODE* root = SmartScreenRectAllocNode();
    root->m_rect = result;

    uint32_t maxSteps = SmartScreenRectGridPos(rect);

    for (uint32_t step = 0; step < maxSteps; step++) {
        BFSNODE* node = s_searchQueue.Head();

        if (!node) {
            break;
        }

        for (uint32_t i = 0; i < NUM_TESTDIRECTIONS; i++) {
            if (node->m_direction != TEST_INVALID && node->m_direction == s_searchDirections[start][i]) {
                continue;
            }

            if (SmartScreenRectClampToScreen(nullptr, node->m_rect, 1)) {
                continue;
            }

            int32_t direction = s_searchDirections[start][i];

            if (direction == TEST_INVALID) {
                continue;
            }

            CRect moved = s_moveFunctions[direction](grid, node->m_rect);

            if (node->m_rect.minY == moved.minY && node->m_rect.minX == moved.minX && node->m_rect.maxY == moved.maxY && node->m_rect.maxX == moved.maxX) {
                result = moved;
                SmartScreenRectRecycleNodes();
                return result;
            }

            BFSNODE* child = SmartScreenRectAllocNode();
            s_searchQueue.LinkToTail(child);
            child->m_rect = moved;
        }

        s_freeNodes.LinkToTail(node);
    }

    result = rect;
    SmartScreenRectRecycleNodes();

    return result;
}

// VERIFY: 392e628c auto
// OFFSET: 0x00615C50
static C2Vector SmartScreenRectFindPosition(SCREENRECTGRIDS grid, const CRect& rect, int32_t* changed) {
    CRect found = SmartScreenRectFindFreeRect(grid, rect);

    *changed = found.minY != rect.minY || found.minX != rect.minX || found.maxY != rect.maxY || found.maxX != rect.maxX;

    C2Vector position;
    position.x = (found.maxX + found.minX) * 0.5f;
    position.y = found.minY;

    return position;
}

// VERIFY: 9ea2ce49 manual
// OFFSET: 0x00615CD0
void SmartScreenRectPlace(SCREENRECTGRIDS grid, CRect& rect) {
    float height = fabsf(rect.maxY - rect.minY);
    float halfWidth = fabsf(rect.maxX - rect.minX) * 0.5f;
    float halfHeight = height * 0.5f;
    float screenWidth = NDCToDDCWidth(1.0f);
    float screenHeight = NDCToDDCHeight(1.0f);
    SmartScreenRectClipRect(rect);

    int32_t changed;
    C2Vector point = SmartScreenRectFindPosition(grid, rect, &changed);

    float minX = halfWidth > point.x ? halfWidth : point.x;
    point.x = screenWidth - halfWidth <= minX ? screenWidth - halfWidth : minX;

    float minY = halfHeight > point.y ? halfHeight : point.y;
    point.y = screenHeight - halfHeight <= minY ? screenHeight - halfHeight : minY;

    rect.minY = point.y;
    rect.minX = point.x - halfWidth;
    rect.maxY = point.y - height;
    rect.maxX = point.x + halfWidth;
    SmartScreenRectClipRect(rect);

    *s_grids[grid].New() = rect;
}

// VERIFY: c9083189 manual
// OFFSET: 0x00615E10
void SmartScreenRectPositionFrame(SCREENRECTGRIDS grid, CSimpleFrame* frame, CSimpleFrame* parent, const C3Vector& position, int32_t center) {
    float halfWidth = frame->GetWidth();
    float height = frame->GetHeight();
    halfWidth = halfWidth * 0.5f;
    float halfHeight = height * 0.5f;
    float screenWidth = NDCToDDCWidth(1.0f);
    float screenHeight = NDCToDDCHeight(1.0f);

    CRect rect;
    rect.minY = position.y;
    rect.minX = position.x - halfWidth;
    rect.maxY = position.y - height;
    rect.maxX = position.x + halfWidth;
    SmartScreenRectClipRect(rect);

    int32_t changed;
    C2Vector point = SmartScreenRectFindPosition(grid, rect, &changed);

    float minX = halfWidth > point.x ? halfWidth : point.x;
    point.x = screenWidth - halfWidth <= minX ? screenWidth - halfWidth : minX;

    float minY = halfHeight > point.y ? halfHeight : point.y;
    point.y = screenHeight - halfHeight <= minY ? screenHeight - halfHeight : minY;

    rect.minY = point.y;
    rect.minX = point.x - halfWidth;
    rect.maxY = point.y - height;
    rect.maxX = point.x + halfWidth;
    SmartScreenRectClipRect(rect);

    *s_grids[grid].New() = rect;

    frame->MarkFramePointsUnused();
    float depth = position.z - parent->m_layoutDepth;
    frame->SetPoint(center ? FRAMEPOINT_CENTER : FRAMEPOINT_TOP, parent, FRAMEPOINT_BOTTOMLEFT, point.x, point.y, 1);
    frame->SetFrameDepth(depth, true);
}

// VERIFY: 98e897bd auto
// OFFSET: 0x00616060
void SmartScreenRectShutdown() {
    s_searchQueue.Clear();
    s_freeNodes.Clear();

    for (uint32_t i = 0; i < SCREENRECTGRID_CLEAR_COUNT; i++) {
        s_grids[i].Clear();
    }
}
