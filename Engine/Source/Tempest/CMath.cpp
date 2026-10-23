#include <cfloat>
#include <cmath>
#include <cstdint>
#include "Engine/Source/Tempest/CMath.h"

// STATIC: 0x00A9B660
static const uint32_t s_reciprocalTable[1025] = {
    0x00800000, 0x007FC011, 0x007F8041, 0x007F4091, 0x007F0100, 0x007EC190, 0x007E823E, 0x007E430C,
    0x007E03FA, 0x007DC506, 0x007D8632, 0x007D477D, 0x007D08E7, 0x007CCA70, 0x007C8C17, 0x007C4DDD,
    0x007C0FC2, 0x007BD1C6, 0x007B93E8, 0x007B5628, 0x007B1887, 0x007ADB04, 0x007A9D9F, 0x007A6057,
    0x007A232E, 0x0079E623, 0x0079A936, 0x00796C66, 0x00792FB4, 0x0078F31F, 0x0078B6A8, 0x00787A4E,
    0x00783E11, 0x007801F1, 0x0077C5EF, 0x00778A0A, 0x00774E41, 0x00771296, 0x0076D707, 0x00769B95,
    0x0076603F, 0x00762506, 0x0075E9E9, 0x0075AEE9, 0x00757405, 0x0075393D, 0x0074FE92, 0x0074C402,
    0x0074898F, 0x00744F37, 0x007414FB, 0x0073DADB, 0x0073A0D6, 0x007366EE, 0x00732D20, 0x0072F36E,
    0x0072B9D8, 0x0072805C, 0x007246FC, 0x00720DB7, 0x0071D48D, 0x00719B7E, 0x0071628A, 0x007129B1,
    0x0070F0F2, 0x0070B84E, 0x00707FC5, 0x00704756, 0x00700F02, 0x006FD6C8, 0x006F9EA9, 0x006F66A3,
    0x006F2EB8, 0x006EF6E7, 0x006EBF30, 0x006E8793, 0x006E5010, 0x006E18A7, 0x006DE157, 0x006DAA21,
    0x006D7305, 0x006D3C02, 0x006D0519, 0x006CCE49, 0x006C9792, 0x006C60F5, 0x006C2A71, 0x006BF406,
    0x006BBDB4, 0x006B877B, 0x006B515B, 0x006B1B54, 0x006AE565, 0x006AAF90, 0x006A79D2, 0x006A442E,
    0x006A0EA2, 0x0069D92F, 0x0069A3D4, 0x00696E91, 0x00693966, 0x00690454, 0x0068CF5A, 0x00689A78,
    0x006865AE, 0x006830FB, 0x0067FC61, 0x0067C7DF, 0x00679374, 0x00675F21, 0x00672AE6, 0x0066F6C2,
    0x0066C2B5, 0x00668EC1, 0x00665AE3, 0x0066271D, 0x0065F36E, 0x0065BFD6, 0x00658C55, 0x006558EC,
    0x00652599, 0x0064F25E, 0x0064BF39, 0x00648C2B, 0x00645934, 0x00642654, 0x0063F38A, 0x0063C0D7,
    0x00638E3A, 0x00635BB4, 0x00632944, 0x0062F6EB, 0x0062C4A8, 0x0062927B, 0x00626064, 0x00622E64,
    0x0061FC79, 0x0061CAA5, 0x006198E6, 0x0061673E, 0x006135AB, 0x0061042E, 0x0060D2C7, 0x0060A175,
    0x00607039, 0x00603F13, 0x00600E02, 0x005FDD06, 0x005FAC20, 0x005F7B50, 0x005F4A94, 0x005F19EE,
    0x005EE95D, 0x005EB8E1, 0x005E887B, 0x005E5829, 0x005E27EC, 0x005DF7C4, 0x005DC7B1, 0x005D97B3,
    0x005D67CA, 0x005D37F5, 0x005D0835, 0x005CD889, 0x005CA8F2, 0x005C7970, 0x005C4A02, 0x005C1AA8,
    0x005BEB63, 0x005BBC32, 0x005B8D15, 0x005B5E0D, 0x005B2F18, 0x005B0038, 0x005AD16B, 0x005AA2B3,
    0x005A740F, 0x005A457E, 0x005A1701, 0x0059E898, 0x0059BA43, 0x00598C02, 0x00595DD4, 0x00592FBA,
    0x005901B3, 0x0058D3C0, 0x0058A5E0, 0x00587814, 0x00584A5A, 0x00581CB5, 0x0057EF22, 0x0057C1A3,
    0x00579437, 0x005766DE, 0x00573998, 0x00570C65, 0x0056DF45, 0x0056B238, 0x0056853E, 0x00565856,
    0x00562B82, 0x0055FEC0, 0x0055D211, 0x0055A574, 0x005578EA, 0x00554C73, 0x0055200E, 0x0054F3BC,
    0x0054C77C, 0x00549B4E, 0x00546F33, 0x0054432A, 0x00541733, 0x0053EB4F, 0x0053BF7D, 0x005393BC,
    0x0053680E, 0x00533C72, 0x005310E8, 0x0052E56F, 0x0052BA09, 0x00528EB5, 0x00526372, 0x00523841,
    0x00520D22, 0x0051E214, 0x0051B718, 0x00518C2E, 0x00516155, 0x0051368E, 0x00510BD8, 0x0050E134,
    0x0050B6A1, 0x00508C1F, 0x005061AF, 0x00503750, 0x00500D02, 0x004FE2C5, 0x004FB899, 0x004F8E7F,
    0x004F6475, 0x004F3A7D, 0x004F1096, 0x004EE6BF, 0x004EBCFA, 0x004E9345, 0x004E69A1, 0x004E400E,
    0x004E168B, 0x004DED1A, 0x004DC3B8, 0x004D9A68, 0x004D7128, 0x004D47F9, 0x004D1EDA, 0x004CF5CC,
    0x004CCCCE, 0x004CA3E0, 0x004C7B03, 0x004C5236, 0x004C2979, 0x004C00CD, 0x004BD831, 0x004BAFA4,
    0x004B8729, 0x004B5EBD, 0x004B3661, 0x004B0E15, 0x004AE5D9, 0x004ABDAD, 0x004A9591, 0x004A6D85,
    0x004A4589, 0x004A1D9C, 0x0049F5BF, 0x0049CDF2, 0x0049A635, 0x00497E87, 0x004956E9, 0x00492F5A,
    0x004907DB, 0x0048E06B, 0x0048B90B, 0x004891BB, 0x00486A79, 0x00484347, 0x00481C25, 0x0047F511,
    0x0047CE0D, 0x0047A718, 0x00478033, 0x0047595C, 0x00473295, 0x00470BDC, 0x0046E533, 0x0046BE98,
    0x0046980D, 0x00467191, 0x00464B23, 0x004624C5, 0x0045FE75, 0x0045D834, 0x0045B201, 0x00458BDE,
    0x004565C9, 0x00453FC3, 0x004519CC, 0x0044F3E3, 0x0044CE08, 0x0044A83D, 0x0044827F, 0x00445CD1,
    0x00443730, 0x0044119E, 0x0043EC1B, 0x0043C6A5, 0x0043A13F, 0x00437BE6, 0x0043569C, 0x0043315F,
    0x00430C31, 0x0042E712, 0x0042C200, 0x00429CFC, 0x00427807, 0x0042531F, 0x00422E46, 0x0042097A,
    0x0041E4BC, 0x0041C00D, 0x00419B6B, 0x004176D7, 0x00415251, 0x00412DD8, 0x0041096E, 0x0040E511,
    0x0040C0C1, 0x00409C80, 0x0040784C, 0x00405425, 0x0040300D, 0x00400C01, 0x003FE804, 0x003FC413,
    0x003FA031, 0x003F7C5B, 0x003F5893, 0x003F34D8, 0x003F112B, 0x003EED8B, 0x003EC9F8, 0x003EA673,
    0x003E82FB, 0x003E5F90, 0x003E3C32, 0x003E18E1, 0x003DF59D, 0x003DD267, 0x003DAF3D, 0x003D8C20,
    0x003D6911, 0x003D460E, 0x003D2319, 0x003D0030, 0x003CDD54, 0x003CBA85, 0x003C97C3, 0x003C750D,
    0x003C5265, 0x003C2FC9, 0x003C0D3A, 0x003BEAB7, 0x003BC841, 0x003BA5D8, 0x003B837B, 0x003B612B,
    0x003B3EE8, 0x003B1CB1, 0x003AFA86, 0x003AD868, 0x003AB657, 0x003A9451, 0x003A7259, 0x003A506C,
    0x003A2E8C, 0x003A0CB8, 0x0039EAF1, 0x0039C936, 0x0039A787, 0x003985E4, 0x0039644D, 0x003942C3,
    0x00392145, 0x0038FFD2, 0x0038DE6C, 0x0038BD12, 0x00389BC4, 0x00387A82, 0x0038594C, 0x00383822,
    0x00381703, 0x0037F5F1, 0x0037D4EB, 0x0037B3F0, 0x00379301, 0x0037721E, 0x00375147, 0x0037307C,
    0x00370FBC, 0x0036EF08, 0x0036CE5F, 0x0036ADC3, 0x00368D32, 0x00366CAC, 0x00364C32, 0x00362BC4,
    0x00360B61, 0x0035EB0A, 0x0035CABE, 0x0035AA7E, 0x00358A49, 0x00356A1F, 0x00354A01, 0x003529EE,
    0x003509E7, 0x0034E9EB, 0x0034C9FA, 0x0034AA15, 0x00348A3A, 0x00346A6B, 0x00344AA7, 0x00342AEF,
    0x00340B41, 0x0033EB9F, 0x0033CC08, 0x0033AC7B, 0x00338CFA, 0x00336D84, 0x00334E19, 0x00332EB9,
    0x00330F64, 0x0032F01A, 0x0032D0DA, 0x0032B1A6, 0x0032927D, 0x0032735E, 0x0032544A, 0x00323541,
    0x00321643, 0x0031F750, 0x0031D867, 0x0031B989, 0x00319AB6, 0x00317BEE, 0x00315D30, 0x00313E7D,
    0x00311FD4, 0x00310136, 0x0030E2A3, 0x0030C41A, 0x0030A59C, 0x00308728, 0x003068BF, 0x00304A60,
    0x00302C0B, 0x00300DC2, 0x002FEF82, 0x002FD14D, 0x002FB322, 0x002F9502, 0x002F76EC, 0x002F58E0,
    0x002F3ADE, 0x002F1CE7, 0x002EFEFA, 0x002EE117, 0x002EC33F, 0x002EA570, 0x002E87AC, 0x002E69F2,
    0x002E4C42, 0x002E2E9C, 0x002E1100, 0x002DF36E, 0x002DD5E7, 0x002DB869, 0x002D9AF5, 0x002D7D8C,
    0x002D602C, 0x002D42D6, 0x002D258A, 0x002D0848, 0x002CEB10, 0x002CCDE2, 0x002CB0BD, 0x002C93A3,
    0x002C7692, 0x002C598B, 0x002C3C8E, 0x002C1F9A, 0x002C02B0, 0x002BE5D0, 0x002BC8FA, 0x002BAC2D,
    0x002B8F6A, 0x002B72B1, 0x002B5601, 0x002B395B, 0x002B1CBE, 0x002B002B, 0x002AE3A2, 0x002AC722,
    0x002AAAAB, 0x002A8E3E, 0x002A71DA, 0x002A5580, 0x002A3930, 0x002A1CE8, 0x002A00AA, 0x0029E476,
    0x0029C84B, 0x0029AC29, 0x00299010, 0x00297401, 0x002957FB, 0x00293BFE, 0x0029200B, 0x00290421,
    0x0028E840, 0x0028CC68, 0x0028B099, 0x002894D4, 0x00287917, 0x00285D64, 0x002841BA, 0x00282619,
    0x00280A81, 0x0027EEF2, 0x0027D36C, 0x0027B7EF, 0x00279C7B, 0x00278111, 0x002765AF, 0x00274A56,
    0x00272F06, 0x002713BE, 0x0026F880, 0x0026DD4B, 0x0026C21E, 0x0026A6FB, 0x00268BE0, 0x002670CE,
    0x002655C5, 0x00263AC4, 0x00261FCC, 0x002604DD, 0x0025E9F7, 0x0025CF1A, 0x0025B445, 0x00259979,
    0x00257EB5, 0x002563FB, 0x00254948, 0x00252E9F, 0x002513FE, 0x0024F965, 0x0024DED6, 0x0024C44E,
    0x0024A9CF, 0x00248F59, 0x002474EB, 0x00245A86, 0x00244029, 0x002425D5, 0x00240B89, 0x0023F146,
    0x0023D70B, 0x0023BCD8, 0x0023A2AE, 0x0023888C, 0x00236E72, 0x00235461, 0x00233A58, 0x00232057,
    0x0023065F, 0x0022EC6E, 0x0022D287, 0x0022B8A7, 0x00229ECF, 0x00228500, 0x00226B39, 0x0022517A,
    0x002237C3, 0x00221E15, 0x0022046E, 0x0021EAD0, 0x0021D13A, 0x0021B7AC, 0x00219E26, 0x002184A7,
    0x00216B31, 0x002151C3, 0x0021385D, 0x00211F00, 0x002105A9, 0x0020EC5B, 0x0020D315, 0x0020B9D7,
    0x0020A0A1, 0x00208773, 0x00206E4C, 0x0020552E, 0x00203C17, 0x00202308, 0x00200A01, 0x001FF102,
    0x001FD80A, 0x001FBF1B, 0x001FA633, 0x001F8D53, 0x001F747A, 0x001F5BAA, 0x001F42E1, 0x001F2A20,
    0x001F1166, 0x001EF8B4, 0x001EE00A, 0x001EC768, 0x001EAECD, 0x001E963A, 0x001E7DAE, 0x001E652A,
    0x001E4CAD, 0x001E3439, 0x001E1BCB, 0x001E0365, 0x001DEB07, 0x001DD2B0, 0x001DBA61, 0x001DA219,
    0x001D89D9, 0x001D71A0, 0x001D596F, 0x001D4145, 0x001D2922, 0x001D1107, 0x001CF8F3, 0x001CE0E7,
    0x001CC8E2, 0x001CB0E4, 0x001C98EE, 0x001C80FF, 0x001C6917, 0x001C5136, 0x001C395D, 0x001C218B,
    0x001C09C1, 0x001BF1FE, 0x001BDA41, 0x001BC28C, 0x001BAADF, 0x001B9338, 0x001B7B99, 0x001B6401,
    0x001B4C70, 0x001B34E6, 0x001B1D63, 0x001B05E8, 0x001AEE73, 0x001AD706, 0x001ABFA0, 0x001AA840,
    0x001A90E8, 0x001A7997, 0x001A624D, 0x001A4B0A, 0x001A33CE, 0x001A1C99, 0x001A056A, 0x0019EE43,
    0x0019D723, 0x0019C00A, 0x0019A8F8, 0x001991EC, 0x00197AE8, 0x001963EA, 0x00194CF3, 0x00193604,
    0x00191F1B, 0x00190838, 0x0018F15D, 0x0018DA89, 0x0018C3BB, 0x0018ACF4, 0x00189634, 0x00187F7B,
    0x001868C8, 0x0018521C, 0x00183B77, 0x001824D9, 0x00180E42, 0x0017F7B1, 0x0017E126, 0x0017CAA3,
    0x0017B426, 0x00179DB0, 0x00178740, 0x001770D8, 0x00175A75, 0x0017441A, 0x00172DC5, 0x00171776,
    0x0017012E, 0x0016EAED, 0x0016D4B2, 0x0016BE7E, 0x0016A850, 0x00169229, 0x00167C08, 0x001665EE,
    0x00164FDB, 0x001639CD, 0x001623C7, 0x00160DC6, 0x0015F7CD, 0x0015E1D9, 0x0015CBEC, 0x0015B606,
    0x0015A026, 0x00158A4C, 0x00157478, 0x00155EAB, 0x001548E5, 0x00153324, 0x00151D6A, 0x001507B7,
    0x0014F209, 0x0014DC62, 0x0014C6C2, 0x0014B127, 0x00149B93, 0x00148605, 0x0014707D, 0x00145AFC,
    0x00144581, 0x0014300C, 0x00141A9D, 0x00140534, 0x0013EFD2, 0x0013DA76, 0x0013C520, 0x0013AFD0,
    0x00139A86, 0x00138542, 0x00137005, 0x00135ACD, 0x0013459C, 0x00133071, 0x00131B4C, 0x0013062D,
    0x0012F114, 0x0012DC01, 0x0012C6F4, 0x0012B1ED, 0x00129CEC, 0x001287F1, 0x001272FC, 0x00125E0E,
    0x00124925, 0x00123442, 0x00121F65, 0x00120A8E, 0x0011F5BD, 0x0011E0F2, 0x0011CC2D, 0x0011B76D,
    0x0011A2B4, 0x00118E00, 0x00117953, 0x001164AB, 0x00115009, 0x00113B6D, 0x001126D7, 0x00111247,
    0x0010FDBC, 0x0010E937, 0x0010D4B9, 0x0010C03F, 0x0010ABCC, 0x0010975F, 0x001082F7, 0x00106E95,
    0x00105A39, 0x001045E2, 0x00103191, 0x00101D46, 0x00100901, 0x000FF4C1, 0x000FE087, 0x000FCC53,
    0x000FB824, 0x000FA3FB, 0x000F8FD8, 0x000F7BBA, 0x000F67A2, 0x000F5390, 0x000F3F83, 0x000F2B7C,
    0x000F177A, 0x000F037E, 0x000EEF88, 0x000EDB97, 0x000EC7AB, 0x000EB3C6, 0x000E9FE5, 0x000E8C0B,
    0x000E7836, 0x000E6466, 0x000E509C, 0x000E3CD7, 0x000E2918, 0x000E155E, 0x000E01AA, 0x000DEDFB,
    0x000DDA52, 0x000DC6AE, 0x000DB310, 0x000D9F77, 0x000D8BE3, 0x000D7855, 0x000D64CC, 0x000D5149,
    0x000D3DCB, 0x000D2A53, 0x000D16DF, 0x000D0371, 0x000CF009, 0x000CDCA6, 0x000CC948, 0x000CB5EF,
    0x000CA29C, 0x000C8F4E, 0x000C7C06, 0x000C68C2, 0x000C5584, 0x000C424B, 0x000C2F18, 0x000C1BEA,
    0x000C08C1, 0x000BF59D, 0x000BE27E, 0x000BCF65, 0x000BBC51, 0x000BA942, 0x000B9638, 0x000B8334,
    0x000B7034, 0x000B5D3A, 0x000B4A45, 0x000B3755, 0x000B246B, 0x000B1185, 0x000AFEA5, 0x000AEBC9,
    0x000AD8F3, 0x000AC622, 0x000AB356, 0x000AA08F, 0x000A8DCD, 0x000A7B10, 0x000A6859, 0x000A55A6,
    0x000A42F9, 0x000A3050, 0x000A1DAC, 0x000A0B0E, 0x0009F874, 0x0009E5E0, 0x0009D351, 0x0009C0C6,
    0x0009AE41, 0x00099BC0, 0x00098945, 0x000976CE, 0x0009645C, 0x000951F0, 0x00093F88, 0x00092D25,
    0x00091AC7, 0x0009086E, 0x0008F61A, 0x0008E3CB, 0x0008D181, 0x0008BF3B, 0x0008ACFB, 0x00089ABF,
    0x00088889, 0x00087657, 0x0008642A, 0x00085201, 0x00083FDE, 0x00082DBF, 0x00081BA6, 0x00080991,
    0x0007F781, 0x0007E575, 0x0007D36F, 0x0007C16D, 0x0007AF70, 0x00079D78, 0x00078B84, 0x00077995,
    0x000767AB, 0x000755C6, 0x000743E6, 0x0007320A, 0x00072033, 0x00070E60, 0x0006FC93, 0x0006EACA,
    0x0006D905, 0x0006C746, 0x0006B58B, 0x0006A3D4, 0x00069223, 0x00068076, 0x00066ECD, 0x00065D2A,
    0x00064B8B, 0x000639F0, 0x0006285A, 0x000616C9, 0x0006053C, 0x0005F3B4, 0x0005E231, 0x0005D0B2,
    0x0005BF37, 0x0005ADC2, 0x00059C50, 0x00058AE4, 0x0005797C, 0x00056818, 0x000556B9, 0x0005455E,
    0x00053408, 0x000522B7, 0x0005116A, 0x00050021, 0x0004EEDD, 0x0004DD9E, 0x0004CC63, 0x0004BB2C,
    0x0004A9FA, 0x000498CC, 0x000487A3, 0x0004767E, 0x0004655E, 0x00045442, 0x0004432A, 0x00043217,
    0x00042108, 0x00040FFE, 0x0003FEF8, 0x0003EDF6, 0x0003DCF9, 0x0003CC01, 0x0003BB0C, 0x0003AA1C,
    0x00039930, 0x00038849, 0x00037766, 0x00036687, 0x000355AD, 0x000344D7, 0x00033405, 0x00032338,
    0x0003126F, 0x000301AA, 0x0002F0E9, 0x0002E02D, 0x0002CF75, 0x0002BEC1, 0x0002AE12, 0x00029D67,
    0x00028CC0, 0x00027C1D, 0x00026B7F, 0x00025AE4, 0x00024A4E, 0x000239BD, 0x0002292F, 0x000218A6,
    0x00020821, 0x0001F7A0, 0x0001E723, 0x0001D6AA, 0x0001C636, 0x0001B5C5, 0x0001A559, 0x000194F1,
    0x0001848E, 0x0001742E, 0x000163D3, 0x0001537B, 0x00014328, 0x000132D9, 0x0001228E, 0x00011247,
    0x00010204, 0x0000F1C5, 0x0000E18B, 0x0000D154, 0x0000C122, 0x0000B0F3, 0x0000A0C9, 0x000090A3,
    0x00008081, 0x00007062, 0x00006048, 0x00005032, 0x00004020, 0x00003012, 0x00002008, 0x00001002,
    0x00000000
};

// STATIC: 0x00DCE218
static const CSoftFloat s_softMinusOne = CSoftFloat::FromInt(-1);

// STATIC: 0x00DCE224
static const CSoftFloat s_softZero = CSoftFloat::FromInt(0);

// STATIC: 0x00DCE284
static const CSoftFloat s_softHalf = { 0x3F000000 };

// STATIC: 0x00DCE2A8
static const CSoftFloat s_softOne = CSoftFloat::FromInt(1);

// STATIC: 0x009F1970
static const double s_exp2Table[64] = {
    1.0, 1.0108892860517005, 1.0218971486541166, 1.0330248790212284, 1.0442737824274138, 1.0556451783605572,
    1.0671404006768237, 1.0787607977571199, 1.0905077326652577, 1.102382583307841, 1.1143867425958924, 1.1265216186082418,
    1.1387886347566916, 1.1511892299529827, 1.1637248587775775, 1.1763969916502812, 1.189207115002721, 1.202156731452703,
    1.215247359980469, 1.22848053610687, 1.241857812073484, 1.255380757024691, 1.2690509571917332, 1.2828700160787783,
    1.2968395546510096, 1.3109612115247644, 1.3252366431597413, 1.339667524053303, 1.3542555469368927, 1.3690024229745905,
    1.383909881963832, 1.3989796725383112, 1.4142135623730951, 1.42961333839197, 1.4451808069770467, 1.460917794180647,
    1.4768261459394993, 1.4929077282912648, 1.5091644275934228, 1.5255981507445384, 1.5422108254079407, 1.559004400237837,
    1.5759808451078865, 1.593142151342267, 1.6104903319492543, 1.6280274218573478, 1.645755478153965, 1.6636765803267364,
    1.681792830507429, 1.7001063537185235, 1.718619298122478, 1.7373338352737062, 1.7562521603732995, 1.7753764925265212,
    1.7947090750031072, 1.8142521755003989, 1.8340080864093424, 1.8539791250833855, 1.8741676341103, 1.8945759815869656,
    1.9152065613971474, 1.9360617934922943, 1.9571441241754002, 1.978456026387951
};

// STATIC: 0x009F1B70
static const double s_log2MulTable[64] = {
    1.0, 0.9846153846153847, 0.9696969696969697, 0.9552238805970149, 0.9411764705882353, 0.927536231884058,
    0.9142857142857143, 0.9014084507042254, 0.8888888888888888, 0.8767123287671232, 0.8648648648648649, 0.8533333333333334,
    0.8421052631578947, 0.8311688311688312, 0.8205128205128205, 0.810126582278481, 0.8, 0.7901234567901234,
    0.7804878048780488, 0.7710843373493976, 0.7619047619047619, 0.7529411764705882, 0.7441860465116279, 0.735632183908046,
    0.7272727272727273, 0.7191011235955056, 0.7111111111111111, 0.7032967032967034, 0.6956521739130435, 0.6881720430107527,
    0.6808510638297872, 0.6736842105263158, 0.6666666666666666, 0.6597938144329897, 0.6530612244897959, 0.6464646464646465,
    0.64, 0.6336633663366337, 0.6274509803921569, 0.6213592233009708, 0.6153846153846154, 0.6095238095238096,
    0.6037735849056604, 0.5981308411214953, 0.5925925925925926, 0.5871559633027523, 0.5818181818181818, 0.5765765765765766,
    0.5714285714285714, 0.5663716814159292, 0.5614035087719298, 0.5565217391304348, 0.5517241379310345, 0.5470085470085471,
    0.5423728813559322, 0.5378151260504201, 0.5333333333333333, 0.5289256198347108, 0.5245901639344263, 0.5203252032520326,
    0.5161290322580645, 0.512, 0.5079365079365079, 0.5039370078740157
};

// STATIC: 0x009F1D70
static const double s_log2AddTable[64] = {
    0.0, 0.02236781302845451, 0.044394119358453436, 0.06608919045777244, 0.0874628412503394, 0.10852445677816905,
    0.12928301694496647, 0.14974711950468206, 0.16992500144231237, 0.18982455888001723, 0.20945336562894978,
    0.22881869049588088, 0.2479275134435855, 0.2667865406949014, 0.28540221886224837, 0.30378074817710293,
    0.32192809488736235, 0.33985000288462475, 0.3575520046180837, 0.37503943134692475, 0.3923174227787603,
    0.4093909361377018, 0.42626475470209796, 0.4429434958487283, 0.45943161863729726, 0.47573343096639775,
    0.4918530963296747, 0.5077946401986962, 0.5235619560570128, 0.5391588111080314, 0.5545888516776374,
    0.5698556083309478, 0.5849625007211562, 0.5999128421871277, 0.6147098441152082, 0.6293566200796096,
    0.6438561897747247, 0.6582114827517948, 0.6724253419714956, 0.6865005271832184, 0.7004397181410922,
    0.7142455176661227, 0.7279204545631992, 0.7414669864011469, 0.7548875021634686, 0.7681843247769263,
    0.7813597135246596, 0.794415866350106, 0.8073549220576041, 0.8201789624151877, 0.8328900141647416,
    0.8454900509443752, 0.8579809951275721, 0.8703647195834046, 0.8826430493618412, 0.8948177633079435,
    0.9068905956085185, 0.9188632372745945, 0.9307373375628862, 0.9425145053392399, 0.9541963103868752,
    0.965784284662087, 0.9772799234999164, 0.9886846867721658
};

// VERIFY: b2ec8d01 auto
// OFFSET: 0x004C4E10
uint32_t CMath::sqrt_(uint32_t n) {
    uint32_t x;

    if (n <= 0xFF) {
        x = n / 12 + 1;
    } else if (n <= 0xFFFF) {
        x = n / 200 + 21;
    } else {
        x = n / 26743 + 444;
    }

    int32_t delta;

    do {
        uint32_t q = n / x;
        delta = static_cast<int32_t>(x - q) / 2;
        x = static_cast<int32_t>(q + x) / 2;
    } while (delta);

    return x;
}

// VERIFY: de17052a manual
// OFFSET: 0x004C4E90
double CMath::log2_(double x) {
    uint32_t hi = reinterpret_cast<uint32_t*>(&x)[1];
    uint32_t index = (hi >> 14) & 0x3F;

    double mantissa = x;
    reinterpret_cast<uint32_t*>(&mantissa)[1] = (hi & 0xFFFFF) | 0x3FF00000;

    double t = s_log2MulTable[index] * mantissa - 1.0;
    double t2 = t * t;

    if (!(x > 1e-307)) {
        return -HUGE_VAL;
    }

    int32_t exponent = static_cast<int32_t>((hi >> 20) & 0x7FF) - 1023;
    return t * (t2 * ((0.2883070248990067 - 0.22949900023246153 * t) * t2 + (0.48089833404997356 - 0.36067132973951144 * t)) + (1.442695040888937 - 0.7213475204127876 * t)) + (exponent + s_log2AddTable[index]);
}

// VERIFY: 8b38845f manual
// OFFSET: 0x004C4F40
double CMath::exp2_(double x) {
    double biased = x + 1023.0;
    uint32_t exponent = static_cast<uint32_t>(biased) - 1;

    double fraction = biased - exponent;
    uint32_t index = reinterpret_cast<uint32_t*>(&fraction)[1] & 0xFC000;
    reinterpret_cast<uint32_t*>(&fraction)[1] ^= index;

    double scale;
    reinterpret_cast<uint32_t*>(&scale)[0] = 0;
    reinterpret_cast<uint32_t*>(&scale)[1] = exponent << 20;

    double f2 = fraction * fraction;
    double t = s_exp2Table[index >> 14] * scale;

    if (exponent > 0x7FF) {
        return static_cast<int32_t>(exponent) > 0 ? HUGE_VAL : 0.0;
    }

    return t * (f2 * ((fraction * 0.0026811946517564956 + 0.005830032491308937) * f2 + (fraction * 0.06087614283854009 + 0.23603244393034895)) + (fraction * 0.6948749415195616 + 0.9997052445684839));
}


// VERIFY: c2ddb11f manual
// OFFSET: 0x004C5000
bool CMath::solvequad_(float a, float b, float c, float& r0, float& r1) {
    float fourAC = a * c * 4.0f;
    float bb = b * b;

    if (bb <= fourAC) {
        return false;
    }

    float root = sqrt(bb - fourAC);
    float q = (b > 0.0f ? b + root : b - root) * -0.5f;
    float inv = 1.0f / (q * a);
    float x0 = q * (inv * q);
    float x1 = c * (a * inv);

    if (x1 > x0) {
        r0 = x0;
        r1 = x1;
    } else {
        r0 = x1;
        r1 = x0;
    }

    return true;
}

// VERIFY: f11908d7 auto
// OFFSET: 0x004C5090
float CMath::normalizeangle0to2pi_(float angle) {
    double result = fmod(angle, 6.2831854820251465);

    if (result < 0.0) {
        result = result + TWO_PI;
    }

    return static_cast<float>(result);
}

// VERIFY: fc69d5aa auto
// OFFSET: 0x004C50C0
float CMath::normalizeAngleNegPiToPi_(float angle) {
    double result = fmod(angle, 6.2831854820251465);

    if (result < -PI) {
        return static_cast<float>(result + TWO_PI);
    }

    if (result > PI) {
        result = result - TWO_PI;
    }

    return static_cast<float>(result);
}

// VERIFY: a3991bed auto
// OFFSET: 0x006E9360
int32_t IsNaN(double value) {
    return _isnan(value);
}

// VERIFY: 2ff13d8c auto
// OFFSET: 0x00482870
int32_t fequalz(float a, float b, float z) {
    return fabsf(a - b) < z;
}

// VERIFY: 20bdea3e auto
// OFFSET: 0x00482890
bool fequal(float a, float b) {
    return fabsf(a - b) < 0.00000023841858f;
}

// VERIFY: c85c4f7f manual
// OFFSET: 0x00493B60
uint32_t CeilToInt64(float value) {
    return static_cast<uint32_t>(static_cast<int64_t>(value + 0.99995f));
}

// VERIFY: dd4c8f48 manual
// OFFSET: 0x00407930
int32_t FloatToInt64(float value) {
    if (value > 0.0f) {
        return static_cast<int32_t>(static_cast<int64_t>(value));
    }

    return static_cast<int32_t>(value);
}

// VERIFY: 334b4c2a manual
// OFFSET: 0x00482900
bool fnotequal(float a, float b) {
    return !(fabsf(a - b) < 0.00000023841858f);
}

// VERIFY: f84f50e9 manual
// OFFSET: 0x00493B90
uint32_t CMath::fuint_pi(float f) {
    if (f > 0.0f) {
        return static_cast<uint32_t>(f + 0.99995f);
    }

    return static_cast<int32_t>(f);
}

// VERIFY: 1209983f manual
// OFFSET: 0x00577370
int32_t CMath::ftrunc(float f) {
    return static_cast<int32_t>(f);
}

// VERIFY: 5b628ede manual
// OFFSET: 0x005773B0
int32_t CMath::fint_n(float f) {
    if (f > 0.0f) {
        return static_cast<uint32_t>(f + 0.5f);
    }

    return static_cast<int32_t>(f - 0.5f);
}

// VERIFY: 1cf74ffc manual
// OFFSET: 0x004828C0
int32_t fnotequalz(float a, float b, float z) {
    return !(fabsf(a - b) < z);
}

// VERIFY: 4074c5e8 auto
// OFFSET: 0x00482940
float fclamp(float value, float min, float max) {
    if (value < min) {
        return min;
    }

    if (max <= value) {
        return max;
    }

    return value;
}

// VERIFY: 7b233428 manual
// OFFSET: 0x0048BCF0
int32_t RoundToInt64(float value) {
    return static_cast<int32_t>(static_cast<int64_t>(value + 0.5f));
}

// VERIFY: 0a474f8f manual
// OFFSET: 0x00497A90
void ClampInPlace(float& value, float min, float max) {
    value = min > value ? min : (max <= value ? max : value);
}

// VERIFY: 6b3260e6 auto
// OFFSET: 0x004C1CF0
bool CMath::fequal(float a, float b) {
    return fabsf(a - b) < 0.00000095367432f;
}

// VERIFY: 5575931b manual
// OFFSET: 0x006E9320
bool CMath::fnotequal(float a, float b) {
    return !(fabsf(a - b) < 0.00000095367432f);
}

// VERIFY: cc462220 auto
// OFFSET: 0x007156C0
float CMath::normalizeAnglePi(float angle) {
    if (angle > PI) {
        return static_cast<float>(fmod(static_cast<double>(angle + PI), static_cast<double>(TWO_PI)) - PI);
    }

    if (angle < -PI) {
        return static_cast<float>(fmod(static_cast<double>(angle - PI), static_cast<double>(TWO_PI)) + PI);
    }

    return angle;
}

// VERIFY: 680421f4 auto
// OFFSET: 0x00783B10
double CMath::exp_(float x) {
    return CMath::exp2_(x * 1.442695f);
}

// VERIFY: 02bd26de manual
// OFFSET: 0x0095D210
uint32_t CSoftFloat::CountLeadingZeros(uint32_t value) {
    if (!value) {
        return 32;
    }

    uint32_t count = 0;

    while (!(value & 0x80000000)) {
        value <<= 1;
        count++;
    }

    return count;
}

// VERIFY: e9f75104 manual
// OFFSET: 0x0095D230
CSoftFloat operator*(const CSoftFloat& l, const CSoftFloat& r) {
    uint32_t a = l.m_bits;
    uint32_t b = r.m_bits;
    uint32_t sign = (a ^ b) & 0x80000000;
    uint32_t expA = a & 0x7F800000;
    uint32_t expB = b & 0x7F800000;
    uint32_t manA = a & 0x7FFFFF;
    uint32_t manB = b & 0x7FFFFF;

    CSoftFloat result;

    if (manA && manB) {
        int32_t exp = static_cast<int32_t>(expB + expA - 0x3F800000);
        uint32_t product = static_cast<uint32_t>((static_cast<uint64_t>((manB | 0xFF800000) << 8) * ((manA | 0xFF800000) << 8)) >> 32);
        uint32_t carry = product >> 31;
        uint32_t mantissa = ((product >> carry) >> 7) & 0x7FFFFF;

        result.m_bits = ~static_cast<uint32_t>((exp - 0x800000) >> 31) & (mantissa | ((carry << 23) + exp) | sign);
    } else if (expA && expB) {
        int32_t exp = static_cast<int32_t>(expB + expA - 0x3F800000);

        result.m_bits = ~static_cast<uint32_t>((exp - 0x800000) >> 31) & (exp | manB | manA | sign);
    } else {
        result.m_bits = 0;
    }

    return result;
}

// VERIFY: c21f12af manual
// OFFSET: 0x0095D2F0
CSoftFloat operator+(const CSoftFloat& l, const CSoftFloat& r) {
    uint32_t a = l.m_bits;
    uint32_t expA = a & 0x7F800000;

    if (!expA) {
        return r;
    }

    uint32_t b = r.m_bits;
    uint32_t expB = b & 0x7F800000;

    if (!expB) {
        return l;
    }

    int32_t signA = static_cast<int32_t>(a) >> 31;
    int32_t manA = static_cast<int32_t>((((a & 0x7FFFFF) | 0x800000) * 2) ^ signA) - signA;
    int32_t signB = static_cast<int32_t>(b) >> 31;
    int32_t manB = static_cast<int32_t>((((b & 0x7FFFFF) | 0x800000) * 2) ^ signB) - signB;
    int32_t diff = static_cast<int32_t>(expB - expA);
    uint32_t exp;

    if (diff > 0) {
        if (diff >= 0xB800000) {
            return r;
        }

        exp = expB;
        manA >>= static_cast<uint32_t>(diff) >> 23;
    } else {
        if (diff <= -0xB800000) {
            return l;
        }

        exp = expA;
        manB >>= (expA - expB) >> 23;
    }

    int32_t sum = manB + manA;

    CSoftFloat result;

    if (!sum) {
        result.m_bits = 0;
        return result;
    }

    uint32_t sign = static_cast<uint32_t>(sum) & 0x80000000;

    if (sum < 0) {
        sum = -sum;
    }

    int32_t shift = 8 - static_cast<int32_t>(CSoftFloat::CountLeadingZeros(sum));

    if (shift >= 0) {
        sum >>= shift;
    } else {
        sum <<= -shift;
    }

    result.m_bits = (((shift - 1) << 23) + exp) | (static_cast<uint32_t>(sum) & 0x7FFFFF) | sign;
    return result;
}

// VERIFY: 547fd2cc manual
// OFFSET: 0x0095D3F0
CSoftFloat CSoftFloat::Reciprocal(const CSoftFloat& value) {
    uint32_t bits = value.m_bits;
    uint32_t mantissa = bits & 0x7FFFFF;
    uint32_t exp = 0x7E800000 - (bits & 0x7F800000);
    uint32_t high = mantissa << 19;
    uint32_t fraction = high | ((((mantissa << 19) >> 13) | high) >> 13);
    uint32_t index = mantissa >> 13;
    uint32_t base = s_reciprocalTable[index];
    uint32_t delta = static_cast<uint32_t>((static_cast<uint64_t>(base - s_reciprocalTable[index + 1]) * fraction) >> 32);
    int32_t combined = static_cast<int32_t>(base - delta + exp);

    CSoftFloat result;
    result.m_bits = (static_cast<uint32_t>(combined) | (bits & 0x80000000)) & ~static_cast<uint32_t>((combined - 0x800000) >> 31);
    return result;
}

// VERIFY: 687e78c4 manual
// OFFSET: 0x0095D460
CSoftFloat CSoftFloat::Floor(const CSoftFloat& value) {
    uint32_t bits = value.m_bits;
    int32_t exp = static_cast<int32_t>((bits >> 23) & 0xFF) - 127;

    if (exp < 0) {
        return (bits * 2) & static_cast<uint32_t>(static_cast<int32_t>(bits) >> 31) ? s_softMinusOne : s_softZero;
    }

    if (exp >= 23) {
        return value;
    }

    uint32_t mask = static_cast<uint32_t>(static_cast<int32_t>(0x80000000) >> (exp + 8));

    CSoftFloat result;

    if ((bits * 2) & ~static_cast<uint32_t>(static_cast<int32_t>(bits) >> 31)) {
        result.m_bits = bits & mask;
        return result;
    }

    uint32_t mantissa = ((bits & 0x7FFFFF) + ~mask) & mask;

    result.m_bits = ((exp + (mantissa >> 23) + 127) << 23) | (mantissa & 0x7FFFFF) | 0x80000000;
    return result;
}

// VERIFY: d556fd30 manual
// OFFSET: 0x0095D500
CSoftFloat CSoftFloat::Ceil(const CSoftFloat& value) {
    uint32_t bits = value.m_bits;
    int32_t exp = static_cast<int32_t>((bits >> 23) & 0xFF) - 127;

    if (exp < 0) {
        return (bits * 2) & static_cast<uint32_t>(static_cast<int32_t>(bits) >> 31) ? s_softZero : s_softOne;
    }

    if (exp >= 23) {
        return value;
    }

    uint32_t mask = static_cast<uint32_t>(static_cast<int32_t>(0x80000000) >> (exp + 8));

    CSoftFloat result;

    if ((bits * 2) & static_cast<uint32_t>(static_cast<int32_t>(bits) >> 31)) {
        result.m_bits = bits & mask;
        return result;
    }

    uint32_t mantissa = ((bits & 0x7FFFFF) + ~mask) & mask;

    result.m_bits = ((exp + (mantissa >> 23) + 127) << 23) | (mantissa & 0x7FFFFF);
    return result;
}

// VERIFY: d339c8ab manual
// OFFSET: 0x0095D620
CSoftFloat CSoftFloat::FromRatio(int32_t numerator, int32_t denominator) {
    CSoftFloat den = CSoftFloat::FromInt(denominator);
    CSoftFloat num = CSoftFloat::FromInt(numerator);

    if (num.m_bits == den.m_bits) {
        CSoftFloat result;
        result.m_bits = 0x3F800000;
        return result;
    }

    return num * CSoftFloat::Reciprocal(den);
}

// VERIFY: 4edb999a auto
// OFFSET: 0x0095D590
CSoftFloat CSoftFloat::Round(const CSoftFloat& value) {
    return CSoftFloat::Floor(value + s_softHalf);
}

// VERIFY: 4d93df78 manual
// OFFSET: 0x0095D5C0
CSoftFloat CSoftFloat::FromInt(int32_t value) {
    CSoftFloat result;

    if (!value) {
        result.m_bits = 0;
        return result;
    }

    uint32_t sign = static_cast<uint32_t>(value) & 0x80000000;
    int32_t magnitude = value < 0 ? -value : value;
    int32_t zeros = static_cast<int32_t>(CSoftFloat::CountLeadingZeros(magnitude));
    int32_t shift = zeros - 8;

    if (shift >= 0) {
        magnitude <<= shift;
    } else {
        magnitude >>= -shift;
    }

    result.m_bits = ((31 - zeros + 127) << 23) | (static_cast<uint32_t>(magnitude) & 0x7FFFFF) | sign;
    return result;
}

// VERIFY: 82e25c68 manual
// OFFSET: 0x005FE800
void CMath::split_(float x, float* frac, int32_t* whole) {
    if (x > 0.0f) {
        *whole = static_cast<int32_t>(static_cast<int64_t>(x));
    } else {
        *whole = static_cast<int32_t>(x) - 1;
    }

    *frac = x - *whole;
}

// VERIFY: f348e946 auto
// OFFSET: 0x006F7A10
float CMath::sin_(float x) {
    float frac;
    int32_t whole;
    CMath::split_(x * 0.31830987f - 0.5f, &frac, &whole);

    float result = 1.0f - frac * ((6.0f - 4.0f * frac) * frac);

    return (whole & 1) ? -result : result;
}

// VERIFY: eea7f5f0 auto
// OFFSET: 0x008CA080
float OrganicSmooth(float from, float to, float t) {
    return from + (to - from) * ((1.0f - cosf(t * PI)) * 0.5f);
}

// VERIFY: 0fa97e20 auto
// OFFSET: 0x006F7A60
void CMath::sincos_(float x, float* s, float* c) {
    float turns = x * 0.31830987f;
    float frac;
    int32_t whole;
    CMath::split_(turns - 0.5f, &frac, &whole);
    float value = 1.0f - frac * ((6.0f - 4.0f * frac) * frac);
    *s = (whole & 1) ? -value : value;
    CMath::split_(turns, &frac, &whole);
    value = 1.0f - frac * ((6.0f - 4.0f * frac) * frac);
    *c = (whole & 1) ? -value : value;
}

// VERIFY: 7fd316a4 auto
// OFFSET: 0x005C59F0
int32_t Clamp(int32_t value, int32_t min, int32_t max) {
    uint32_t offset = value - min;
    uint32_t range = max - min;

    if (offset > range) {
        return min + (range & ~(static_cast<int32_t>(offset) >> 31));
    }

    return value;
}
