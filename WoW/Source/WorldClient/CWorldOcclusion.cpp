#include "CWorldOcclusion.h"
#include <malloc.h>
#include <cmath>
#include <cstring>
#include "Engine/Source/Tempest/C44Matrix.h"
#include "Engine/Source/Tempest/CAaBox.h"
#include "Engine/Source/Tempest/CAaSphere.h"
#include "Engine/Source/Tempest/CFrustum.h"
#include "Engine/Source/Tempest/CMath.h"
#include "WoW/Source/WorldClient/CPortalView.h"
#include "WoW/Source/WorldClient/CWorldMath.h"
#include "WoW/Source/WorldClient/Map.h"
#include "WoW/Source/WorldClient/Types.h"
#include "WoW/Source/WorldClient/World.h"
#include "WoW/Source/WorldClient/WorldView.h"
#include "WoW/Source/WorldClient/MapChunk.h"
#include "Engine/Source/Gx/Gx.h"
#include "Engine/Source/Gx/CGxBuf.h"
#include "Engine/Source/Gx/CGxDevice/CGxDevice.h"

static const float CBSCALE = 64.0f;

// STATIC: 0x00AEEF60
TSExplicitList<CWorldOccluder, offsetof(CWorldOccluder, m_link)> s_occluderFreeList;

// STATIC: 0x00AEEF6C
TSExplicitList<CWorldAntiOccluder, offsetof(CWorldAntiOccluder, m_link)> s_antiOccluderFreeList;

// STATIC: 0x00D2DCD8
TSGrowableArray<C4Plane> CWorldOcclusion::planeArray;

// STATIC: 0x00D2DCE8
TSGrowableArray<CClipVolume> CWorldOcclusion::clipVolumeArray;

// STATIC: 0x00ADF3F4
static int32_t aabbCornerSelectX[8] = { 0, 1, 1, 0, 0, 1, 1, 0 };

// STATIC: 0x00ADF414
static int32_t aabbCornerSelectY[8] = { 0, 0, 1, 1, 0, 0, 1, 1 };

// STATIC: 0x00ADF434
static int32_t aabbCornerSelectZ[8] = { 0, 0, 0, 0, 1, 1, 1, 1 };

// STATIC: 0x00CD8FD8
static C3Vector s_clipBufferVerts[9];

// STATIC: 0x00D2DCD4
static int32_t s_occlusionVolumesInit;

// STATIC: 0x00AEFCE0
static C3Vector s_occlusionVolumeVerts0[4] = { C3Vector(-8814.79004f, 474.859985f, 0.0f), C3Vector(-8814.79004f, 474.859985f, 138.869995f), C3Vector(-8986.45996f, 679.73999f, 138.869995f), C3Vector(-8986.45996f, 679.73999f, 0.0f) };

// STATIC: 0x00AEF318
static C3Vector s_occlusionVolumeVerts1[8] = { C3Vector(5567.66016f, 714.340027f, 596.679993f), C3Vector(5605.31982f, 394.26001f, 596.679993f), C3Vector(5888.74023f, 382.140015f, 596.679993f), C3Vector(6026.97998f, 494.670013f, 596.679993f), C3Vector(6051.12012f, 634.179993f, 596.679993f), C3Vector(5952.54004f, 849.710022f, 596.679993f), C3Vector(5793.60986f, 991.539978f, 596.679993f), C3Vector(5582.06006f, 799.390015f, 596.679993f) };

// STATIC: 0x00AEF378
static C3Vector s_occlusionVolumeVerts2[6] = { C3Vector(5634.5f, 419.0f, 630.400024f), C3Vector(5612.56982f, 814.0f, 630.400024f), C3Vector(5810.18994f, 953.830017f, 630.400024f), C3Vector(5955.66992f, 785.400024f, 630.400024f), C3Vector(6025.93018f, 545.390015f, 630.400024f), C3Vector(5817.91016f, 394.350006f, 630.400024f) };

// STATIC: 0x00AEF3C0
static C3Vector s_occlusionVolumeVerts3[4] = { C3Vector(5892.49023f, 627.299988f, 620.0f), C3Vector(5892.49023f, 627.299988f, 664.0f), C3Vector(5990.5f, 557.700012f, 664.0f), C3Vector(5990.5f, 557.700012f, 620.0f) };

// STATIC: 0x00AEF3F0
static C3Vector s_occlusionVolumeVerts4[4] = { C3Vector(5847.0f, 539.0f, 620.0f), C3Vector(5847.0f, 539.0f, 685.0f), C3Vector(5800.0f, 526.0f, 685.0f), C3Vector(5800.0f, 526.0f, 620.0f) };

// STATIC: 0x00AEF420
static C3Vector s_occlusionVolumeVerts5[4] = { C3Vector(5827.39014f, 603.349976f, 620.340027f), C3Vector(5827.39014f, 603.349976f, 679.340027f), C3Vector(5786.06006f, 590.070007f, 678.059998f), C3Vector(5786.06006f, 590.070007f, 620.059998f) };

// STATIC: 0x00AEF450
static C3Vector s_occlusionVolumeVerts6[4] = { C3Vector(5730.99023f, 679.309998f, 620.76001f), C3Vector(5730.99023f, 679.309998f, 670.76001f), C3Vector(5679.87012f, 720.150024f, 670.76001f), C3Vector(5679.87012f, 720.150024f, 620.76001f) };

// STATIC: 0x00AEF480
static C3Vector s_occlusionVolumeVerts7[4] = { C3Vector(5774.5f, 655.429993f, 620.150024f), C3Vector(5774.5f, 655.429993f, 661.150024f), C3Vector(5762.91992f, 611.929993f, 661.150024f), C3Vector(5762.91992f, 611.929993f, 620.150024f) };

// STATIC: 0x00AEF4B0
static C3Vector s_occlusionVolumeVerts8[4] = { C3Vector(5784.6499f, 664.969971f, 620.02002f), C3Vector(5784.6499f, 664.969971f, 658.02002f), C3Vector(5823.0f, 686.909973f, 658.02002f), C3Vector(5823.0f, 686.909973f, 620.02002f) };

// STATIC: 0x00AEF4E0
static C3Vector s_occlusionVolumeVerts9[4] = { C3Vector(5841.56006f, 633.280029f, 620.659973f), C3Vector(5841.56006f, 633.280029f, 669.659973f), C3Vector(5845.91992f, 669.590027f, 664.719971f), C3Vector(5845.91992f, 669.590027f, 620.719971f) };

// STATIC: 0x00AEF510
static C3Vector s_occlusionVolumeVerts10[4] = { C3Vector(5904.16992f, 618.48999f, 620.409973f), C3Vector(5904.16992f, 618.48999f, 672.409973f), C3Vector(5883.91992f, 591.090027f, 672.359985f), C3Vector(5883.91992f, 591.090027f, 620.359985f) };

// STATIC: 0x00AEF540
static C3Vector s_occlusionVolumeVerts11[4] = { C3Vector(5895.04004f, 627.820007f, 620.552979f), C3Vector(5895.04004f, 627.820007f, 649.552979f), C3Vector(5834.77979f, 553.960022f, 649.552979f), C3Vector(5834.77979f, 553.960022f, 620.552979f) };

// STATIC: 0x00AEF570
static C3Vector s_occlusionVolumeVerts12[4] = { C3Vector(5899.41016f, 668.109985f, 620.539978f), C3Vector(5899.41016f, 668.109985f, 676.539978f), C3Vector(5941.06006f, 714.98999f, 661.169983f), C3Vector(5941.06006f, 714.98999f, 620.169983f) };

// STATIC: 0x00AEF5A0
static C3Vector s_occlusionVolumeVerts13[4] = { C3Vector(5864.97021f, 751.169983f, 620.590027f), C3Vector(5864.97021f, 751.169983f, 669.590027f), C3Vector(5835.75f, 720.710022f, 669.590027f), C3Vector(5835.75f, 720.710022f, 620.590027f) };

// STATIC: 0x00AEF5D0
static C3Vector s_occlusionVolumeVerts14[5] = { C3Vector(5636.4502f, 398.959991f, 630.0f), C3Vector(5947.9502f, 827.400024f, 630.0f), C3Vector(5924.75f, 796.799988f, 574.109985f), C3Vector(5806.25f, 636.309998f, 397.399994f), C3Vector(5636.4502f, 398.959991f, 574.109985f) };

// STATIC: 0x00AEF60C
static C3Vector s_occlusionVolumeVerts15[5] = { C3Vector(5590.91016f, 783.859985f, 630.0f), C3Vector(6006.8999f, 537.080017f, 630.0f), C3Vector(5971.75f, 552.690002f, 558.75f), C3Vector(5806.25f, 636.309998f, 397.399994f), C3Vector(5631.60986f, 751.340027f, 527.849976f) };

// STATIC: 0x00AEF648
static C3Vector s_occlusionVolumeVerts16[4] = { C3Vector(5749.8501f, 865.609985f, 630.0f), C3Vector(5749.8501f, 865.609985f, 704.0f), C3Vector(5864.16992f, 851.469971f, 704.0f), C3Vector(5864.16992f, 851.469971f, 630.0f) };

// STATIC: 0x00AEF678
static C3Vector s_occlusionVolumeVerts17[4] = { C3Vector(5800.50977f, 816.419983f, 630.0f), C3Vector(5800.50977f, 816.419983f, 704.0f), C3Vector(5814.83984f, 906.580017f, 704.0f), C3Vector(5814.83984f, 906.580017f, 630.0f) };

// STATIC: 0x00AEF6A8
static C3Vector s_occlusionVolumeVerts18[4] = { C3Vector(5868.95996f, 572.640015f, 623.440002f), C3Vector(5868.95996f, 572.640015f, 665.440002f), C3Vector(5847.00977f, 539.190002f, 665.440002f), C3Vector(5847.00977f, 539.190002f, 623.440002f) };

// STATIC: 0x00AEF6D8
static C3Vector s_occlusionVolumeVerts19[4] = { C3Vector(5867.43018f, 720.049988f, 624.900024f), C3Vector(5867.43018f, 720.049988f, 654.900024f), C3Vector(5893.14014f, 700.97998f, 654.690002f), C3Vector(5893.14014f, 700.97998f, 624.690002f) };

// STATIC: 0x00AEF708
static C3Vector s_occlusionVolumeVerts20[7] = { C3Vector(2478.83008f, -5620.12988f, 366.5f), C3Vector(2491.94995f, -5599.3999f, 366.5f), C3Vector(2487.5f, -5574.9502f, 366.5f), C3Vector(2466.23999f, -5561.56006f, 366.5f), C3Vector(2441.82007f, -5566.62988f, 366.5f), C3Vector(2433.40991f, -5612.20996f, 366.5f), C3Vector(2454.25f, -5625.66016f, 366.5f) };

// STATIC: 0x00AEF75C
static C3Vector s_occlusionVolumeVerts21[4] = { C3Vector(2510.16992f, -5702.89014f, 364.0f), C3Vector(2491.44995f, -5658.31982f, 520.0f), C3Vector(2429.59009f, -5523.72021f, 520.0f), C3Vector(2413.67993f, -5483.43994f, 364.0f) };

// STATIC: 0x00AEF790
static C3Vector s_occlusionVolumeVerts22[7] = { C3Vector(2370.94995f, -5680.14014f, 382.0f), C3Vector(2351.13989f, -5635.16016f, 382.0f), C3Vector(2377.80005f, -5472.43994f, 382.0f), C3Vector(2512.1499f, -5448.47021f, 382.0f), C3Vector(2586.20996f, -5551.41992f, 382.0f), C3Vector(2537.37988f, -5704.72998f, 382.0f), C3Vector(2425.21997f, -5744.56006f, 382.0f) };

// STATIC: 0x00AEF7E4
static C3Vector s_occlusionVolumeVerts23[5] = { C3Vector(2364.06006f, -5692.39014f, 382.23999f), C3Vector(2347.53003f, -5672.2002f, 382.23999f), C3Vector(2333.75f, -5647.47021f, 382.23999f), C3Vector(2353.04004f, -5636.87988f, 382.23999f), C3Vector(2382.53003f, -5679.75977f, 382.23999f) };

// STATIC: 0x00AEF820
static C3Vector s_occlusionVolumeVerts24[4] = { C3Vector(1204.95996f, -4963.31982f, 0.0f), C3Vector(1198.35999f, -4933.68994f, 346.079987f), C3Vector(1158.87f, -4823.16992f, 344.76001f), C3Vector(1149.31006f, -4791.83008f, 0.0f) };

// STATIC: 0x00AEF850
static C3Vector s_occlusionVolumeVerts25[4] = { C3Vector(1219.40002f, -5004.41992f, 0.0f), C3Vector(1219.40002f, -5004.41992f, 173.610001f), C3Vector(1128.43005f, -4743.66016f, 173.130005f), C3Vector(1128.43005f, -4743.66016f, 0.0f) };

// STATIC: 0x00AEF880
static C3Vector s_occlusionVolumeVerts26[7] = { C3Vector(1111.31006f, -4819.8999f, 172.490005f), C3Vector(1090.0f, -4809.75977f, 172.490005f), C3Vector(1073.63f, -4794.27002f, 172.490005f), C3Vector(1069.92004f, -4767.41016f, 172.490005f), C3Vector(1098.58997f, -4757.35986f, 172.490005f), C3Vector(1163.56006f, -4754.04004f, 172.490005f), C3Vector(1167.65002f, -4794.54004f, 172.490005f) };

// STATIC: 0x00AEF8D4
static C3Vector s_occlusionVolumeVerts27[5] = { C3Vector(1142.31995f, -5006.37988f, 172.119995f), C3Vector(1153.18005f, -4968.25f, 172.119995f), C3Vector(1254.25f, -4932.14014f, 172.119995f), C3Vector(1286.37f, -4988.77002f, 172.119995f), C3Vector(1156.18005f, -5028.95996f, 172.119995f) };

// STATIC: 0x00AEF910
static C3Vector s_occlusionVolumeVerts28[4] = { C3Vector(2181.38989f, 5312.52979f, -50.0f), C3Vector(2181.38989f, 5312.52979f, 44.9700012f), C3Vector(2116.82007f, 5332.81006f, 44.9700012f), C3Vector(2116.82007f, 5332.81006f, -50.0f) };

// STATIC: 0x00AEF940
static C3Vector s_occlusionVolumeVerts29[4] = { C3Vector(2116.82007f, 5332.81006f, -50.0f), C3Vector(2116.82007f, 5332.81006f, 44.9700012f), C3Vector(2100.21997f, 5284.27002f, 44.9700012f), C3Vector(2100.21997f, 5284.27002f, -50.0f) };

// STATIC: 0x00AEF970
static C3Vector s_occlusionVolumeVerts30[4] = { C3Vector(2324.45996f, 5188.87988f, -50.0f), C3Vector(2324.45996f, 5188.87988f, 38.2900009f), C3Vector(2273.20996f, 5209.70996f, 38.2900009f), C3Vector(2273.20996f, 5209.70996f, -50.0f) };

// STATIC: 0x00AEF9A0
static C3Vector s_occlusionVolumeVerts31[4] = { C3Vector(2275.62988f, 5190.16992f, -50.0f), C3Vector(2275.62988f, 5190.16992f, 39.5800018f), C3Vector(2286.95996f, 5210.31006f, 39.5800018f), C3Vector(2286.95996f, 5210.31006f, -50.0f) };

// STATIC: 0x00AEF9D0
static C3Vector s_occlusionVolumeVerts32[4] = { C3Vector(2878.06006f, 6177.6001f, 207.710007f), C3Vector(2937.51001f, 6113.31982f, 207.710007f), C3Vector(2905.16992f, 6082.3999f, 207.710007f), C3Vector(2845.54004f, 6145.41992f, 207.710007f) };

// STATIC: 0x00AEFA00
static C3Vector s_occlusionVolumeVerts33[4] = { C3Vector(2763.38989f, 6089.56982f, 207.809998f), C3Vector(2828.62988f, 6147.74023f, 207.809998f), C3Vector(2797.19995f, 6179.5498f, 207.809998f), C3Vector(2733.96997f, 6120.93018f, 207.809998f) };

// STATIC: 0x00AEFA30
static C3Vector s_occlusionVolumeVerts34[4] = { C3Vector(2741.27002f, 6261.16016f, 207.809998f), C3Vector(2799.12012f, 6196.66992f, 207.809998f), C3Vector(2831.20996f, 6228.18994f, 207.809998f), C3Vector(2773.36011f, 6292.16992f, 207.809998f) };

// STATIC: 0x00AEFA60
static C3Vector s_occlusionVolumeVerts35[4] = { C3Vector(2902.29004f, 6274.68018f, 202.809998f), C3Vector(2846.67993f, 6224.75977f, 202.809998f), C3Vector(2880.3999f, 6195.16992f, 202.809998f), C3Vector(2931.47998f, 6241.72998f, 202.809998f) };

// STATIC: 0x00AEFA90
static C3Vector s_occlusionVolumeVerts36[4] = { C3Vector(2926.83008f, 6120.47021f, 79.0500031f), C3Vector(2781.04004f, 6278.5498f, 79.0800018f), C3Vector(2772.78003f, 6291.58008f, 203.899994f), C3Vector(2935.22998f, 6114.39014f, 192.479996f) };

// STATIC: 0x00AEFAC0
static C3Vector s_occlusionVolumeVerts37[4] = { C3Vector(2853.65991f, 6199.45996f, 202.5f), C3Vector(2910.86011f, 6255.33008f, 200.339996f), C3Vector(2885.68994f, 6230.22998f, 74.6900024f), C3Vector(2860.12012f, 6206.81006f, 82.6900024f) };

// STATIC: 0x00AEFAF0
static C3Vector s_occlusionVolumeVerts38[4] = { C3Vector(2826.04004f, 6175.62012f, 80.3700027f), C3Vector(2760.6499f, 6116.41992f, 78.8600006f), C3Vector(2747.80005f, 6104.56982f, 203.550003f), C3Vector(2826.79004f, 6176.22021f, 203.270004f) };

// STATIC: 0x00AEFB20
static C3Vector s_occlusionVolumeVerts39[5] = { C3Vector(2924.91992f, 6123.45996f, 191.494003f), C3Vector(2940.1001f, 6198.47021f, 191.494003f), C3Vector(2914.90991f, 6257.2002f, 191.494003f), C3Vector(2860.75f, 6282.54004f, 191.494003f), C3Vector(2781.79004f, 6276.06982f, 191.494003f) };

// STATIC: 0x00AEFB5C
static C3Vector s_occlusionVolumeVerts40[4] = { C3Vector(3968.21997f, -3566.95996f, 256.790009f), C3Vector(3968.21997f, -3566.95996f, 377.160004f), C3Vector(3940.76001f, -3451.53003f, 354.329987f), C3Vector(3940.76001f, -3451.53003f, 291.480011f) };

// STATIC: 0x00AEFB8C
static C3Vector s_occlusionVolumeVerts41[4] = { C3Vector(4792.56982f, -1936.70996f, 160.0f), C3Vector(4792.56982f, -1936.70996f, 295.200012f), C3Vector(4791.64014f, -2034.21997f, 295.200012f), C3Vector(4791.64014f, -2034.21997f, 160.0f) };

// STATIC: 0x00AEFBBC
static C3Vector s_occlusionVolumeVerts42[4] = { C3Vector(4790.41016f, -2159.31006f, 295.200012f), C3Vector(4790.41016f, -2159.31006f, 160.0f), C3Vector(4791.3501f, -2056.95996f, 160.0f), C3Vector(4791.3501f, -2056.95996f, 295.200012f) };

// STATIC: 0x00AEFD88
static C3Vector s_occlusionVolumeVerts43[5] = { C3Vector(-208.169998f, -661.929993f, 120.120003f), C3Vector(-232.25f, -655.880005f, 120.120003f), C3Vector(-262.429993f, -661.539978f, 120.120003f), C3Vector(-261.609985f, -693.070007f, 120.120003f), C3Vector(-220.75f, -701.130005f, 120.120003f) };

// STATIC: 0x00AEFDC4
static C3Vector s_occlusionVolumeVerts44[4] = { C3Vector(-336.98999f, -570.869995f, 46.5400009f), C3Vector(-327.170013f, -684.76001f, 46.5400009f), C3Vector(-510.790009f, -626.059998f, 46.5400009f), C3Vector(-467.160004f, -575.200012f, 46.5400009f) };

// STATIC: 0x00AEFDF4
static C3Vector s_occlusionVolumeVerts45[4] = { C3Vector(-336.720001f, -777.349976f, 0.0f), C3Vector(-336.720001f, -777.349976f, 126.599998f), C3Vector(-335.709991f, -707.359985f, 126.599998f), C3Vector(-335.709991f, -707.359985f, 0.0f) };

// STATIC: 0x00AEFE24
static C3Vector s_occlusionVolumeVerts46[4] = { C3Vector(-364.899994f, -637.22998f, 0.0f), C3Vector(-364.899994f, -637.22998f, 85.5299988f), C3Vector(-365.040009f, -806.450012f, 87.6800003f), C3Vector(-365.040009f, -806.450012f, 0.0f) };

// STATIC: 0x00AEFE54
static C3Vector s_occlusionVolumeVerts47[4] = { C3Vector(-227.259995f, -760.090027f, 0.0f), C3Vector(-227.259995f, -760.090027f, 101.339996f), C3Vector(-444.859985f, -740.530029f, 91.1800003f), C3Vector(-444.859985f, -740.530029f, 0.0f) };

// STATIC: 0x00AEFE84
static C3Vector s_occlusionVolumeVerts48[5] = { C3Vector(-287.790009f, -547.27002f, 6.36000013f), C3Vector(-227.240005f, -735.280029f, 6.36000013f), C3Vector(-393.959991f, -846.23999f, 6.36000013f), C3Vector(-499.179993f, -683.280029f, 6.36000013f), C3Vector(-441.809998f, -599.640015f, 6.36000013f) };

// STATIC: 0x00AEFBEC
static C3Vector s_occlusionVolumeVerts49[4] = { C3Vector(1530.72998f, -3570.53003f, 158.300003f), C3Vector(1418.81995f, -3136.56006f, 158.300003f), C3Vector(1418.81995f, -3136.56006f, 0.0f), C3Vector(1530.72998f, -3570.53003f, 0.0f) };

// STATIC: 0x00AEFC1C
static C3Vector s_occlusionVolumeVerts50[4] = { C3Vector(1579.05005f, -3461.44995f, 147.199997f), C3Vector(1535.65002f, -3238.25f, 147.199997f), C3Vector(1447.47998f, -3239.18994f, 147.199997f), C3Vector(1467.60999f, -3591.0f, 147.199997f) };

// STATIC: 0x00AEFC50
static C3Vector s_occlusionVolumeVerts51[7] = { C3Vector(1713.95996f, -3423.56006f, 133.839996f), C3Vector(1637.57996f, -3417.65991f, 133.839996f), C3Vector(1608.12f, -3365.8999f, 133.839996f), C3Vector(1608.5f, -3307.29004f, 133.839996f), C3Vector(1635.44995f, -3263.93994f, 133.839996f), C3Vector(1687.90002f, -3252.33008f, 133.839996f), C3Vector(1747.5f, -3271.53003f, 133.839996f) };

// STATIC: 0x00AEFCA4
static C3Vector s_occlusionVolumeVerts52[5] = { C3Vector(1747.57996f, -3461.09009f, 139.0f), C3Vector(1658.70996f, -3446.07007f, 139.0f), C3Vector(1625.67004f, -3548.93994f, 139.0f), C3Vector(1676.44995f, -3612.45996f, 139.0f), C3Vector(1720.95996f, -3582.86011f, 139.0f) };

// STATIC: 0x00AEFD10
static C3Vector s_occlusionVolumeVerts53[5] = { C3Vector(301.839996f, -475.679993f, 104.580002f), C3Vector(293.690002f, -509.899994f, 104.580002f), C3Vector(307.109985f, -528.01001f, 104.580002f), C3Vector(485.309998f, -525.929993f, 104.580002f), C3Vector(496.529999f, -493.970001f, 104.580002f) };

// STATIC: 0x00AEFD4C
static C3Vector s_occlusionVolumeVerts54[5] = { C3Vector(481.859985f, -273.23999f, 103.43f), C3Vector(499.190002f, -264.26001f, 103.43f), C3Vector(490.119995f, -233.919998f, 103.43f), C3Vector(304.619995f, -229.970001f, 103.43f), C3Vector(302.910004f, -268.570007f, 103.43f) };

// STATIC: 0x00AEFEC0
static C3Vector s_occlusionVolumeVerts55[4] = { C3Vector(2087.0f, 300.0f, 503.0f), C3Vector(2087.0f, -300.0f, 503.0f), C3Vector(2087.0f, -300.0f, -2000.0f), C3Vector(2087.0f, 300.0f, -2000.0f) };

// STATIC: 0x00AEFEF0
static C3Vector s_occlusionVolumeVerts56[4] = { C3Vector(2087.0f, -300.0f, 500.0f), C3Vector(2531.0f, -288.0f, 500.0f), C3Vector(2531.0f, -288.0f, -2000.0f), C3Vector(2087.0f, -300.0f, -2000.0f) };

// STATIC: 0x00AEFF20
static C3Vector s_occlusionVolumeVerts57[4] = { C3Vector(2087.0f, 300.0f, 500.0f), C3Vector(2538.0f, 212.0f, 500.0f), C3Vector(2538.0f, 212.0f, -2000.0f), C3Vector(2087.0f, 300.0f, -2000.0f) };

// STATIC: 0x00AEFF50
static C3Vector s_occlusionVolumeVerts58[4] = { C3Vector(9000.0f, -1300.0f, 10000.0f), C3Vector(9000.0f, -1300.0f, -10000.0f), C3Vector(-9000.0f, -1300.0f, -10000.0f), C3Vector(-9000.0f, -1300.0f, 10000.0f) };

// STATIC: 0x00AEFF80
static C3Vector s_occlusionVolumeVerts59[8] = { C3Vector(-274.299988f, 2212.69995f, 175.0f), C3Vector(-316.200012f, 2120.5f, 175.0f), C3Vector(-435.899994f, 2033.80005f, 175.0f), C3Vector(-549.5f, 2104.6001f, 175.0f), C3Vector(-613.599976f, 2212.30005f, 175.0f), C3Vector(-547.400024f, 2320.6001f, 175.0f), C3Vector(-438.100006f, 2382.30005f, 175.0f), C3Vector(-326.899994f, 2320.5f, 175.0f) };

// STATIC: 0x00AEFFE0
static C3Vector s_occlusionVolumeVerts60[4] = { C3Vector(-470.600006f, 2243.30005f, 614.0f), C3Vector(-520.099976f, 2293.3999f, -163.899994f), C3Vector(-336.100006f, 2116.0f, -163.300003f), C3Vector(-405.700012f, 2179.5f, 614.0f) };

// STATIC: 0x00AF0010
static C3Vector s_occlusionVolumeVerts61[4] = { C3Vector(-475.600006f, 2173.80005f, 614.0f), C3Vector(-502.200012f, 2148.69995f, -158.0f), C3Vector(-344.399994f, 2309.0f, -158.0f), C3Vector(-403.899994f, 2243.69995f, 614.0f) };

// STATIC: 0x00AF0040
static OcclusionVolume s_occlusionVolumes[62] = {
    { 0, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts0, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts1, 8 } },
    { 571, 2, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts2, 6 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts3, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts4, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts5, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts6, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts7, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts8, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts9, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts10, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts11, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts12, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts13, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts14, 5 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts15, 5 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts16, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts17, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts18, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts19, 4 } },
    { 609, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts20, 7 } },
    { 609, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts21, 4 } },
    { 609, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts22, 7 } },
    { 609, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts23, 5 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts24, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts25, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts26, 7 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts27, 5 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts28, 4 } },
    { 571, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts29, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts30, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts31, 4 } },
    { 571, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts32, 4 } },
    { 571, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts33, 4 } },
    { 571, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts34, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts35, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts36, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts37, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts38, 4 } },
    { 571, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts39, 5 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts40, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts41, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts42, 4 } },
    { 600, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts43, 5 } },
    { 600, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts44, 4 } },
    { 600, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts45, 4 } },
    { 600, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts46, 4 } },
    { 600, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts47, 4 } },
    { 600, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts48, 5 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts49, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts50, 4 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts51, 7 } },
    { 571, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts52, 5 } },
    { 575, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts53, 5 } },
    { 575, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts54, 5 } },
    { 603, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts55, 4 } },
    { 603, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts56, 4 } },
    { 603, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts57, 4 } },
    { 631, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts58, 4 } },
    { 631, 0, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts59, 8 } },
    { 631, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts60, 4 } },
    { 631, 1, { CAaBox(C3Vector(0.0f, 0.0f, 0.0f), C3Vector(0.0f, 0.0f, 0.0f)), s_occlusionVolumeVerts61, 4 } },
};

// STATIC: 0x00AEEF78
static C3Vector s_worldOccluderVerts0[22] = { C3Vector(2287.72998f, 5169.47998f, 44.1699982f), C3Vector(2297.80005f, 5165.72021f, 44.1699982f), C3Vector(2296.86011f, 5166.18994f, 33.6300011f), C3Vector(2328.37988f, 5159.77002f, 33.6300011f), C3Vector(2327.02002f, 5159.72998f, 45.0f), C3Vector(2337.20996f, 5163.87988f, 45.0f), C3Vector(2335.61011f, 5162.58984f, 30.8999996f), C3Vector(2364.8999f, 5187.66992f, 30.8999996f), C3Vector(2364.08008f, 5186.95996f, 39.1500015f), C3Vector(2372.45996f, 5196.06006f, 39.1500015f), C3Vector(2372.95996f, 5192.8999f, 27.8099995f), C3Vector(2381.71997f, 5230.60986f, 27.8099995f), C3Vector(2381.62012f, 5229.2002f, 40.9000015f), C3Vector(2381.37988f, 5239.99023f, 40.9000015f), C3Vector(2370.56006f, 5272.83984f, 47.0f), C3Vector(2365.47998f, 5281.81006f, 47.0f), C3Vector(2366.57007f, 5281.31006f, 33.0999985f), C3Vector(2342.16992f, 5301.83984f, 33.0999985f), C3Vector(2341.38989f, 5302.18994f, 50.6300011f), C3Vector(2325.29004f, 5313.29004f, 50.6300011f), C3Vector(2325.29004f, 5313.29004f, 50.6300011f), C3Vector(2314.61011f, 5303.97021f, 50.6300011f) };

// STATIC: 0x00AEF080
static C3Vector s_worldOccluderVerts1[16] = { C3Vector(2189.1001f, 5348.62988f, 44.7400017f), C3Vector(2200.34009f, 5343.43018f, 44.7400017f), C3Vector(2122.22998f, 5368.75977f, 32.3600006f), C3Vector(2076.05005f, 5340.02002f, 32.3600006f), C3Vector(2076.05005f, 5340.02002f, 32.3600006f), C3Vector(2064.67993f, 5300.58984f, 32.3600006f), C3Vector(2065.1001f, 5302.50977f, 64.3199997f), C3Vector(2063.46997f, 5290.35986f, 64.3199997f), C3Vector(2062.59009f, 5291.6001f, 51.3300018f), C3Vector(2077.09009f, 5252.00977f, 51.3300018f), C3Vector(2077.09009f, 5252.00977f, 51.3300018f), C3Vector(2109.76001f, 5225.79004f, 51.3300018f), C3Vector(2108.5f, 5228.31006f, 39.7599983f), C3Vector(2190.55005f, 5195.1001f, 39.7599983f), C3Vector(2189.1001f, 5348.62988f, 44.7400017f), C3Vector(2200.34009f, 5343.43018f, 44.7400017f) };

// STATIC: 0x00AEF140
static C3Vector s_worldOccluderVerts2[6] = { C3Vector(701.409973f, -5045.22021f, 25.0f), C3Vector(702.150024f, -5032.52002f, 25.0f), C3Vector(702.320007f, -5034.81006f, 15.8100004f), C3Vector(704.030029f, -4994.79004f, 15.8100004f), C3Vector(703.460022f, -4995.75f, 19.4300003f), C3Vector(701.070007f, -4983.97998f, 19.4300003f) };

// STATIC: 0x00AEF188
static C3Vector s_worldOccluderVerts3[6] = { C3Vector(693.52002f, -4946.7002f, 20.2999992f), C3Vector(690.320007f, -4934.54004f, 19.3600006f), C3Vector(691.159973f, -4936.68994f, 15.3999996f), C3Vector(675.849976f, -4907.31006f, 15.3999996f), C3Vector(676.570007f, -4907.72021f, 28.3799992f), C3Vector(670.090027f, -4896.62012f, 28.3799992f) };

// STATIC: 0x00AEF1D0
static C3Vector s_worldOccluderVerts4[22] = { C3Vector(3922.27002f, -3780.83008f, 223.880005f), C3Vector(3931.12988f, -3816.6499f, 225.300003f), C3Vector(3930.42993f, -3816.25f, 247.050003f), C3Vector(3972.51001f, -3865.33008f, 246.880005f), C3Vector(3985.02002f, -3862.97998f, 266.380005f), C3Vector(4019.30005f, -3871.8501f, 266.380005f), C3Vector(4018.84009f, -3870.28003f, 283.140015f), C3Vector(4057.41992f, -3866.94995f, 283.600006f), C3Vector(4057.41992f, -3866.94995f, 283.600006f), C3Vector(4090.54004f, -3839.40991f, 283.140015f), C3Vector(4097.93018f, -3813.61011f, 312.859985f), C3Vector(4108.39014f, -3773.71997f, 310.559998f), C3Vector(4108.39014f, -3773.71997f, 310.559998f), C3Vector(4085.13989f, -3743.48999f, 312.859985f), C3Vector(4070.07007f, -3716.09009f, 279.630005f), C3Vector(4032.63989f, -3689.23999f, 283.549988f), C3Vector(4032.63989f, -3689.23999f, 283.549988f), C3Vector(4005.66992f, -3693.69995f, 279.630005f), C3Vector(4003.37988f, -3697.04004f, 266.73999f), C3Vector(3963.34009f, -3709.25f, 240.449997f), C3Vector(3963.34009f, -3709.25f, 240.449997f), C3Vector(3936.93994f, -3745.72998f, 230.220001f) };

// STATIC: 0x00D2DC20
static WorldOccluderData s_worldOccluders[5] = {
    WorldOccluderData(571, -100.0f, s_worldOccluderVerts0, 22),
    WorldOccluderData(571, -100.0f, s_worldOccluderVerts1, 16),
    WorldOccluderData(571, -50.0f, s_worldOccluderVerts2, 6),
    WorldOccluderData(571, -50.0f, s_worldOccluderVerts3, 6),
    WorldOccluderData(571, -100.0f, s_worldOccluderVerts4, 22)
};

// VERIFY: f52cc1f6 manual
// OFFSET: 0x005EEB70
void CWorldOccluder::Initialize() {
}

// VERIFY: 4db2afc6 manual
// OFFSET: 0x0078F6A0
void CWorldOcclusion::UpdateBufferChunk(const C3Vector* verts, const float* heights, const uint32_t* indices, int32_t count, const C3Vector* offset, int32_t holes) {
    if (!(CWorld::enables & Enable_Culling) || CWorldScene::camVec.z < -0.89999998f || CWorldScene::camVec.z > 0.89999998f) {
        return;
    }

    C3Vector* projected = s_clipBufferVerts;

    for (int32_t i = 0; i < count; i++) {
        projected->x = verts[*indices].x + offset->x;
        projected->y = verts[*indices].y + offset->y;
        projected->z = heights[*indices] + offset->z;
        *projected = *projected * CWorldScene::s_clipBufferMatrix;

        float inv = 1.0f / projected->z;
        projected++;
        indices++;
        projected[-1].x = projected[-1].x * inv;
        projected[-1].y = inv * projected[-1].y;
    }

    if (holes) {
        for (int32_t i = 0; i < count - 1; i++) {
            int32_t x0 = CMath::fint_(s_clipBufferVerts[i].x * CBSCALE - 0.5f) + 192;
            int32_t x1 = CMath::fint_(s_clipBufferVerts[i + 1].x * CBSCALE - 0.5f) + 192;

            if (x1 < x0) {
                int32_t swap = x0;
                x0 = x1;
                x1 = swap;
            }

            if (x0 < 0) {
                x0 = 0;
            }

            if (x1 >= 384) {
                x1 = 383;
            }

            for (; x0 <= x1; x0++) {
                if (!(CWorldScene::s_clipBufferMask[x0] & 1)) {
                    CWorldScene::s_clipBuffer[x0] = -1000001.0f;
                }
            }
        }

        return;
    }

    for (int32_t i = 0; i < count - 1; i++) {
        const C3Vector& p0 = s_clipBufferVerts[i];
        const C3Vector& p1 = s_clipBufferVerts[i + 1];

        if (p0.z < 0.027777778f || p1.z < 0.027777778f) {
            continue;
        }

        float height = p1.y <= p0.y ? p1.y : p0.y;
        int32_t x0 = CMath::fint_(p0.x * CBSCALE - 0.5f) + 192;
        int32_t x1 = CMath::fint_(p1.x * CBSCALE - 0.5f) + 192;

        if (x1 < x0) {
            int32_t swap = x0;
            x0 = x1;
            x1 = swap;
        }

        if (x0 < 0) {
            x0 = 0;
        }

        if (x1 >= 384) {
            x1 = 383;
        }

        for (; x0 <= x1; x0++) {
            if (height > CWorldScene::s_clipBuffer[x0]) {
                CWorldScene::s_clipBuffer[x0] = height;
            }
        }
    }
}

// VERIFY: 5b9f55f1 manual
// OFFSET: 0x0078F900
void CWorldOcclusion::UpdateBuffer(const C3Vector* points, int32_t count, uint32_t flags) {
    if (!(CWorld::enables & Enable_Culling) || CWorldScene::camVec.z < -0.89999998f || CWorldScene::camVec.z > 0.89999998f) {
        return;
    }

    C3Vector* projected = s_clipBufferVerts;

    for (int32_t i = 0; i < count; i++) {
        *projected = *points * CWorldScene::s_clipBufferMatrix;

        float inv = 1.0f / projected->z;

        if (inv > 0.0f) {
            projected->x = projected->x * inv;
            projected->y = inv * projected->y;
        }

        points++;
        projected++;
    }

    for (int32_t i = 0; i < count - 1; i++) {
        const C3Vector& p0 = s_clipBufferVerts[i];
        const C3Vector& p1 = s_clipBufferVerts[i + 1];

        if (p0.z < 0.027777778f || p1.z < 0.027777778f) {
            continue;
        }

        float height = p1.y <= p0.y ? p1.y : p0.y;
        int32_t x0 = CMath::fint_(p0.x * CBSCALE - 0.5f) + 192;
        int32_t x1 = CMath::fint_(p1.x * CBSCALE - 0.5f) + 192;

        if (x1 < x0) {
            int32_t swap = x0;
            x0 = x1;
            x1 = swap;
        }

        if (x0 < 0) {
            x0 = 0;
        }

        if (x1 >= 384) {
            x1 = 383;
        }

        if (flags & 1) {
            if (x0 <= x1) {
                memset(&CWorldScene::s_clipBufferMask[x0], 1, x1 - x0 + 1);

                for (int32_t x = x0; x <= x1; x++) {
                    if (height > CWorldScene::s_clipBuffer[x]) {
                        CWorldScene::s_clipBuffer[x] = height;
                    }
                }
            }
        } else {
            for (int32_t x = x0; x <= x1; x++) {
                if (height > CWorldScene::s_clipBuffer[x]) {
                    CWorldScene::s_clipBuffer[x] = height;
                }
            }
        }
    }
}

// VERIFY: a0f63ed4 manual
// OFFSET: 0x0078FC40
uint32_t CWorldOcclusion::QueryBuffer(const C3Vector& center, float radius, uint8_t flags) {
    if (!(CWorld::enables & Enable_Culling) || fabs(radius) < 2.3841858e-07f || CWorldScene::camVec.z < -0.89999998f || CWorldScene::camVec.z > 0.89999998f) {
        return 0;
    }

    C3Vector view = center * CWorldScene::s_clipBufferMatrix;
    C3Vector extent = C3Vector(radius, radius, 0.0f) * CWorldScene::projMatrix;

    if (!(flags & 0x8) && view.z < 50.0f) {
        return 0;
    }

    float invZ = 1.0f / view.z;
    view.x *= invZ;
    extent.x *= invZ;
    extent.y *= invZ;
    view.y = extent.x + invZ * view.y;

    int32_t first = CMath::fint_((view.x - extent.y) * CBSCALE - 0.5f) + 192;
    int32_t last = CMath::fint_((extent.y + view.x) * CBSCALE - 0.5f) + 193;

    if (first >= 384 || last < 0) {
        return 0;
    }

    if (first < 0) {
        first = 0;
    }

    if (last >= 384) {
        last = 383;
    }

    for (int32_t i = first; i <= last; i++) {
        if (view.y > CWorldScene::s_clipBuffer[i]) {
            return 0;
        }
    }

    return 2;
}

// VERIFY: 3ac5bc19 auto
// OFFSET: 0x0078FDC0
uint32_t CWorldOcclusion::QueryBuffer(CAaBox* box, uint8_t flags) {
    if (!(CWorld::enables & Enable_Culling) || CWorldScene::camVec.z < -0.89999998f || CWorldScene::camVec.z > 0.89999998f) {
        return 0;
    }

    float minX = 3.4028235e38f;
    float maxX = -3.4028235e38f;
    float maxY = -3.4028235e38f;
    const C3Vector* corners[2] = { &box->b, &box->t };

    for (uint32_t i = 0; i < 8; i++) {
        C3Vector corner(corners[aabbCornerSelectX[i]]->x, corners[aabbCornerSelectY[i]]->y, corners[aabbCornerSelectZ[i]]->z);
        corner = corner * CWorldScene::s_clipBufferMatrix;

        if (!(flags & 0x8) && corner.z < 50.0f) {
            return 0;
        }

        float scale = 1.0f / corner.z;
        float x = corner.x * scale;
        float y = scale * corner.y;

        if (minX > x) {
            minX = x;
        }

        if (maxX < x) {
            maxX = x;
        }

        if (maxY < y) {
            maxY = y;
        }
    }

    int32_t first = CMath::fint_(minX * CBSCALE - 0.5f) + 192;
    int32_t last = CMath::fint_(maxX * CBSCALE - 0.5f) + 193;

    if (first >= 384 || last < 0) {
        return 0;
    }

    if (first < 0) {
        first = 0;
    }

    if (last >= 384) {
        last = 383;
    }

    for (int32_t i = first; i <= last; i++) {
        if (maxY > CWorldScene::s_clipBuffer[i]) {
            return 0;
        }
    }

    return 2;
}

// VERIFY: c855a200 auto
// OFFSET: 0x007CC810
void CWorldScene::AddWorldOccluders() {
    for (uint32_t i = 0; i < 5; i++) {
        WorldOccluderData& data = s_worldOccluders[i];

        if (data.m_mapId != s_mapId || CWorldScene::FrustumCull(&data.m_bounds)) {
            continue;
        }

        for (uint32_t j = 0; j < data.m_count; j += 2) {
            CWorldView::AddOccluder(data.m_verts[j], data.m_verts[j + 1]);
        }
    }
}

// VERIFY: be6bb79d auto
// OFFSET: 0x007CC880
void CWorldOccluder::ClipBufferUpdate() {
    CWorldOcclusion::UpdateBuffer(this->m_points, 2, 1);
}

// VERIFY: 7e61d360 manual
// OFFSET: 0x007CC890
WorldOccluderData::WorldOccluderData(int32_t mapId, float depth, C3Vector* verts, uint32_t count) {
    this->m_mapId = mapId;
    this->m_bounds.b = C3Vector(3.4028235e38f, 3.4028235e38f, 3.4028235e38f);
    this->m_bounds.t = C3Vector(-3.4028235e38f, -3.4028235e38f, -3.4028235e38f);
    this->m_verts = verts;
    this->m_count = count;

    for (uint32_t i = 0; i < this->m_count; i++) {
        const C3Vector& vert = verts[i];

        if (this->m_bounds.b.x > vert.x) {
            this->m_bounds.b.x = vert.x;
        }

        if (this->m_bounds.b.y > vert.y) {
            this->m_bounds.b.y = vert.y;
        }

        if (this->m_bounds.t.x < vert.x) {
            this->m_bounds.t.x = vert.x;
        }

        if (this->m_bounds.t.y < vert.y) {
            this->m_bounds.t.y = vert.y;
        }

        if (this->m_bounds.t.z < vert.z) {
            this->m_bounds.t.z = vert.z;
        }

        float bottom = vert.z + depth;

        if (bottom < this->m_bounds.b.z) {
            this->m_bounds.b.z = bottom;
        }
    }
}

// VERIFY: 8b33fdfb manual
// OFFSET: 0x007CC9A0
CWorldOccluder* CWorldOccluder::Alloc() {
    CWorldOccluder* occluder = s_occluderFreeList.Head();

    if (!occluder) {
        occluder = s_occluderFreeList.NewNode(2, 0, 0);
    }

    occluder->m_link.Unlink();

    return occluder;
}

// VERIFY: 21308e7d manual
// OFFSET: 0x007CCA90
void CWorldOccluder::Recycle() {
    this->m_link.Unlink();
    s_occluderFreeList.LinkToTail(this);
}

// VERIFY: 23d792fd manual
// OFFSET: 0x007CCD20
static void InitOcclusionVolumes() {
    if (s_occlusionVolumesInit) {
        return;
    }

    for (uint32_t i = 0; i < 62; i++) {
        OcclusionVolumeData& data = s_occlusionVolumes[i].m_data;
        const C3Vector* pos = data.m_positions;

        for (int32_t j = 0; j < static_cast<int32_t>(data.m_positionCount); j++, pos++) {
            if (data.m_bounds.b.x > pos->x) {
                data.m_bounds.b.x = pos->x;
            }

            if (data.m_bounds.b.y > pos->y) {
                data.m_bounds.b.y = pos->y;
            }

            if (data.m_bounds.b.z > pos->z) {
                data.m_bounds.b.z = pos->z;
            }

            if (data.m_bounds.t.x < pos->x) {
                data.m_bounds.t.x = pos->x;
            }

            if (data.m_bounds.t.y < pos->y) {
                data.m_bounds.t.y = pos->y;
            }

            if (data.m_bounds.t.z < pos->z) {
                data.m_bounds.t.z = pos->z;
            }
        }
    }

    s_occlusionVolumesInit = 1;
    CWorldOcclusion::planeArray.m_chunk = 32;
    CWorldOcclusion::clipVolumeArray.m_chunk = 32;
}

// VERIFY: 58f32a53 auto
// OFFSET: 0x007CCDF0
uint32_t CWorldOcclusion::GetClipVolumeCount() {
    return CWorldOcclusion::clipVolumeArray.Count();
}

// VERIFY: 96f106cd manual
// OFFSET: 0x007CCE00
uint32_t CWorldOcclusion::QueryVolumes(CAaSphere* sphere) {
    for (uint32_t i = 0; i < CWorldOcclusion::clipVolumeArray.Count(); i++) {
        CClipVolume& volume = CWorldOcclusion::clipVolumeArray[i];
        C4Plane* planes = &CWorldOcclusion::planeArray[volume.m_planeStart];
        uint32_t j;

        for (j = 0; j < volume.m_planeCount; j++) {
            if (planes[j].n.z * sphere->c.z + planes[j].n.x * sphere->c.x + sphere->c.y * planes[j].n.y + planes[j].d + sphere->r > 0.0f) {
                break;
            }
        }

        if (j == volume.m_planeCount) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 9dd72105 manual
// OFFSET: 0x007CCFA0
uint32_t CWorldOcclusion::QueryVolumes(C3Vector* points, uint32_t count) {
    for (uint32_t i = 0; i < CWorldOcclusion::clipVolumeArray.Count(); i++) {
        CClipVolume& volume = CWorldOcclusion::clipVolumeArray[i];
        C4Plane* planes = &CWorldOcclusion::planeArray[volume.m_planeStart];
        uint32_t j;

        for (j = 0; j < volume.m_planeCount; j++) {
            uint32_t k;

            for (k = 0; k < count; k++) {
                if (points[k].z * planes[j].n.z + points[k].x * planes[j].n.x + planes[j].n.y * points[k].y + planes[j].d > 0.0f) {
                    break;
                }
            }

            if (k < count) {
                break;
            }
        }

        if (j == volume.m_planeCount) {
            return 1;
        }
    }

    return 0;
}

// VERIFY: 931f6673 manual
// OFFSET: 0x007CD4E0
void CWorldOcclusion::AddVolume(const C3Vector* eye, const C3Vector* verts, int32_t count, int32_t clipToView) {
    uint32_t planeStart = CWorldOcclusion::planeArray.Count();
    uint32_t planeCount = 0;

    if (clipToView && CWorldScene::frustumPortalView.m_maxViewDepth > 0.000001f) {
        float depth = CWorldScene::frustumPortalView.m_maxViewDepth;
        C4Plane viewPlane;
        viewPlane.n = CWorldScene::camVec;
        viewPlane.d = -((depth * CWorldScene::camVec.z + eye->z) * CWorldScene::camVec.z + CWorldScene::camVec.y * (CWorldScene::camVec.y * depth + eye->y) + CWorldScene::camVec.x * (CWorldScene::camVec.x * depth + eye->x));

        C3Vector* clipped = static_cast<C3Vector*>(alloca(sizeof(C3Vector) * 2 * count));
        uint32_t clippedCount = 0;
        CWorldMath::ClipVertexListToPlane(verts, count, clipped, clippedCount, viewPlane, 0.000099999997f, 1);

        if (clippedCount <= 2) {
            return;
        }

        for (uint32_t i = 0; i < clippedCount; i++) {
            const C3Vector& a = clipped[i];
            const C3Vector& b = clipped[(i + 1) % clippedCount];
            C3Vector toEye(eye->x - a.x, eye->y - a.y, eye->z - a.z);
            C3Vector edge(b.x - a.x, b.y - a.y, b.z - a.z);

            C4Plane plane;
            plane.n.x = edge.y * toEye.z - edge.z * toEye.y;
            plane.n.y = edge.z * toEye.x - toEye.z * edge.x;
            plane.n.z = edge.x * toEye.y - toEye.x * edge.y;
            plane.d = 0.0f;

            float lengthSq = plane.n.y * plane.n.y + plane.n.z * plane.n.z + plane.n.x * plane.n.x;

            if (lengthSq > 0.000099999997f) {
                float inv = 1.0f / sqrtf(lengthSq);
                plane.n.x = plane.n.x * inv;
                plane.n.y = plane.n.y * inv;
                plane.n.z = inv * plane.n.z;
                plane.d = -(plane.n.y * a.y + plane.n.z * a.z + plane.n.x * a.x);
                CWorldOcclusion::planeArray.Add(1, &plane);
                planeCount++;
            }
        }
    } else {
        if (count <= 0) {
            return;
        }

        for (int32_t i = 0; i < count; i++) {
            C4Plane* plane = CWorldOcclusion::planeArray.New();
            plane->From3Pos(verts[i], verts[(i + 1) % count], *eye);
            planeCount++;
        }
    }

    if (!planeCount) {
        return;
    }

    C4Plane* cap = CWorldOcclusion::planeArray.New();
    cap->From3Pos(verts[0], verts[1], verts[2]);
    planeCount++;

    if (cap->n.z * eye->z + cap->n.y * eye->y + eye->x * cap->n.x + cap->d < 0.0f) {
        for (uint32_t i = 0; i < planeCount; i++) {
            C4Plane& plane = CWorldOcclusion::planeArray.m_data[planeStart + i];
            plane.d = -plane.d;
            plane.n.x = -plane.n.x;
            plane.n.y = -plane.n.y;
            plane.n.z = -plane.n.z;
        }
    }

    CClipVolume volume;
    volume.m_planeStart = planeStart;
    volume.m_planeCount = planeCount;
    CWorldOcclusion::clipVolumeArray.Add(1, &volume);
}

// VERIFY: 10cf9e8a auto
// OFFSET: 0x007CD850
void CWorldScene::CreateOcclusionVolumes(const C3Vector* eye, const C3Vector* corners, int32_t onlyFlagged) {
    CWorldOcclusion::planeArray.SetCount(0);
    CWorldOcclusion::clipVolumeArray.SetCount(0);

    if (!s_occlusionVolumesInit) {
        InitOcclusionVolumes();
    }

    CFrustum frustum;
    frustum.CalcPlanesFromCorners(corners);

    for (uint32_t i = 0; i < 62; i++) {
        OcclusionVolume& volume = s_occlusionVolumes[i];

        if (s_mapId != volume.m_mapID || (onlyFlagged && !(volume.m_flags & 2))) {
            continue;
        }

        if (frustum.Cull(volume.m_data.m_bounds)) {
            CWorldOcclusion::AddVolume(eye, volume.m_data.m_positions, volume.m_data.m_positionCount, !(volume.m_flags & 1));
        }
    }
}

// VERIFY: 49cb931b auto
// OFFSET: 0x007CD910
void CWorldOcclusion::ClearVolumes() {
    CWorldOcclusion::planeArray.SetCount(0);
    CWorldOcclusion::clipVolumeArray.SetCount(0);
}

// VERIFY: 1ad39c5b manual
// OFFSET: 0x007CD930
void CWorldScene::RenderOcclusionVolumes() {
    if (!(CWorld::enables & Enable_PortalVis)) {
        return;
    }

    int32_t vertexCount = 0;
    int32_t indexCount = 0;

    for (uint32_t i = 0; i < 62; i++) {
        if (s_occlusionVolumes[i].m_mapID == s_mapId) {
            int32_t count = s_occlusionVolumes[i].m_data.m_positionCount;
            vertexCount += count;
            indexCount += 3 * (count - 2);
        }
    }

    if (!vertexCount || !indexCount) {
        return;
    }

    CImVector color;
    color.b = 0x20;
    color.g = 0x80;
    color.r = 0xFF;
    color.a = 0x8F;

    C44Matrix world;
    world.Translate(C3Vector(-CWorldScene::s_activeWorldView.x, -CWorldScene::s_activeWorldView.y, -CWorldScene::s_activeWorldView.z));
    g_theGxDevicePtr->XformPushAndSet(GxXform_World, world);

    g_theGxDevicePtr->RsPush();
    GxRsSet(GxRs_BlendingMode, 2);
    GxUpdateAlphaRef(GxRs_AlphaRef);
    GxRsSet(GxRs_DepthWrite, 0);
    GxRsSet(GxRs_Lighting, 0);
    GxRsSet(GxRs_Fog, 0);
    GxRsSet(GxRs_PixelShader, 0);
    GxRsSet(GxRs_VertexShader, 0);
    GxRsSet(GxRs_MatDiffuse, *reinterpret_cast<int32_t*>(&color));
    GxRsSet(GxRs_Culling, 0);

    CGxBuf* vertexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Vertex, sizeof(CGxVertexPN), vertexCount);
    CGxVertexPN* vertex = reinterpret_cast<CGxVertexPN*>(g_theGxDevicePtr->BufLock(vertexBuf));

    for (uint32_t i = 0; i < 62; i++) {
        OcclusionVolumeData& data = s_occlusionVolumes[i].m_data;

        if (s_occlusionVolumes[i].m_mapID != s_mapId) {
            continue;
        }

        const C3Vector* positions = data.m_positions;
        C3Vector edgeA(positions[1].x - positions[0].x, positions[1].y - positions[0].y, positions[1].z - positions[0].z);
        C3Vector edgeB(positions[2].x - positions[0].x, positions[2].y - positions[0].y, positions[2].z - positions[0].z);
        C3Vector normal = C3Vector::Cross(edgeA, edgeB);

        for (int32_t j = 0; j < static_cast<int32_t>(data.m_positionCount); j++) {
            vertex->m_position = data.m_positions[j];
            vertex->m_normal = normal;
            vertex++;
        }
    }

    g_theGxDevicePtr->BufUnlock(vertexBuf, 0);
    vertexBuf->m_hasData = 1;
    GxPrimVertexPtr(vertexBuf, GxVBF_PN);

    CGxBuf* indexBuf = g_theGxDevicePtr->BufStream(GxPoolTarget_Index, sizeof(uint16_t), indexCount);
    uint16_t* indices = reinterpret_cast<uint16_t*>(g_theGxDevicePtr->BufLock(indexBuf));
    int32_t base = 0;

    for (uint32_t i = 0; i < 62; i++) {
        OcclusionVolume& volume = s_occlusionVolumes[i];

        if (volume.m_mapID != s_mapId) {
            continue;
        }

        for (int32_t j = 0; j < static_cast<int32_t>(volume.m_data.m_positionCount) - 2; j++) {
            indices[0] = static_cast<uint16_t>(base);
            indices[1] = static_cast<uint16_t>(base + j + 1);
            indices[2] = static_cast<uint16_t>(base + j + 2);
            indices += 3;
        }

        base += volume.m_data.m_positionCount;
    }

    g_theGxDevicePtr->BufUnlock(indexBuf, 0);
    indexBuf->m_hasData = 1;
    g_theGxDevicePtr->PrimIndexPtr(indexBuf);

    CGxBatch batch;
    batch.m_primType = GxPrim_Triangles;
    batch.m_start = 0;
    batch.m_count = indexCount;
    batch.m_minIndex = 0;
    batch.m_maxIndex = static_cast<uint16_t>(vertexCount);
    g_theGxDevicePtr->Draw(&batch, 1);

    g_theGxDevicePtr->RsPop();
    GxXformPop(GxXform_World);
}
