global constexpr u32 RAND_TABLE[] =
	{
		0x1941d918, 0x1da69f45, 0x0d3a8c3f, 0x1f739b8d, 0x193657bc, 0x1790a54b, 0x19cc70e0, 0x0f03f023, 0x1a585b66, 0x19fa4872, 0x1e08595e, 0x1d9c5f5a, 0x01b6b527, 0x0e43f478, 0x0c387187, 0x012b8ed1, 0x0360903a, 0x167994bb,
		0x09f3bdb7, 0x012040a3, 0x01fe6eca, 0x1f07702b, 0x1bc32119, 0x0608d8e9, 0x02e20404, 0x0fe0feaa, 0x151c86cb, 0x0fb530df, 0x0da69c8c, 0x157c1f5f, 0x1293efe4, 0x03f58a5d, 0x0a7e41ad, 0x1d8c3e29, 0x0a0e4395, 0x1004e4ae,
		0x0bab1d3a, 0x1d9cd2b4, 0x0fb3b141, 0x1fd56e3e, 0x0ed295ed, 0x18b18254, 0x039dd876, 0x063db56c, 0x120833f7, 0x16794822, 0x1efc0562, 0x074d5079, 0x087b6e65, 0x04d8bda0, 0x11d3ee85, 0x11d85054, 0x10cd4cc9, 0x0ed7bd3a,
		0x143ab993, 0x1a0f2403, 0x00d2f361, 0x0be0e98c, 0x1db08015, 0x094a7d94, 0x0bc8a8ff, 0x0051d416, 0x1d06409c, 0x0ecb1abb, 0x1947444b, 0x138cc9db, 0x09bad3c5, 0x1d74181e, 0x0319320b, 0x0655902c, 0x04375b94, 0x0bb3722e,
		0x1c3e1e34, 0x199cb03f, 0x171a7d40, 0x0898c972, 0x03aa79d6, 0x1f0f5bd2, 0x1eb32e5b, 0x11e68b14, 0x04ae15d4, 0x119e0550, 0x0500ee0a, 0x07bd963c, 0x147f9045, 0x15834bea, 0x11263ebe, 0x13a6cfb2, 0x083709ec, 0x13111e9a,
		0x1aab6140, 0x0b50d84e, 0x1d837eed, 0x0ae0652d, 0x1d215e24, 0x038b288c, 0x13b2baf3, 0x00eb9caa, 0x061ca2ed, 0x109b5d26, 0x09970bfb, 0x178d9dea, 0x00d749f4, 0x1eb742c0, 0x163dc62e, 0x15e026cb, 0x053bd571, 0x1c511803,
		0x1ca95bcc, 0x05a118c9, 0x09aea3bc, 0x01c981ba, 0x15a7e5f6, 0x14c2c6c7, 0x0cadb6dd, 0x0f8de22d, 0x1f13ee0d, 0x0370fa1f, 0x167dd4fd, 0x12cd9cad, 0x00b9ca8b, 0x084ead84, 0x0697cac7, 0x09892fa2, 0x1271fdd7, 0x03b64ee0,
		0x14653541, 0x058f8277, 0x04431047, 0x1f90ad33, 0x0697fe86, 0x1f596d12, 0x0d08b54c, 0x1f2b6f8b, 0x1ef2ede5, 0x141d7828, 0x1d7ac4d1, 0x1675c3a3, 0x06f281e7, 0x0b9a6a61, 0x0dfa4ce9, 0x07294add, 0x196f3ae8, 0x05d8d979,
		0x088c6dc7, 0x04bc65a8, 0x17dd1de2, 0x1ca3ec3d, 0x1d4b9711, 0x1d2789bc, 0x102a1ee5, 0x15718c4c, 0x1ff8b155, 0x11e09231, 0x0616addb, 0x1fd452a9, 0x146895d8, 0x0c79bef9, 0x07cd319c, 0x1e95000e, 0x046fe9c1, 0x1130d06e,
		0x09e4f80c, 0x175662a0, 0x11060b36, 0x15f28113, 0x1435eead, 0x14d79c59, 0x088c69f9, 0x0f4fe784, 0x06a207f3, 0x0c26fd00, 0x000ac7d4, 0x0829bcd1, 0x0a97b0ae, 0x1ae5f809, 0x1da564cf, 0x1274e2de, 0x0b92ca9e, 0x0766e8de,
		0x0b285b9e, 0x0e2d9035, 0x123b1323, 0x1be88567, 0x12a8db43, 0x0acf96a2, 0x0779a714, 0x01f7bf86, 0x1ca950c8, 0x10350a0c, 0x0037338b, 0x15bd6b01, 0x140c28fe, 0x13b33683, 0x0198aaa5, 0x0b481b01, 0x05b54f6a, 0x1107b722,
		0x1b572f5f, 0x06b89398, 0x0193f1fc, 0x1804993f, 0x048aa5f7, 0x16acd6e3, 0x09a2e83b, 0x18bc63ac, 0x10f4b7bf, 0x07dac789, 0x00c963e4, 0x07844217, 0x0d252621, 0x1fc1792b, 0x05082a9c, 0x0adb9bd7, 0x0ee8244d, 0x04faa91e,
		0x07084378, 0x198964e7, 0x02d0a88c, 0x10652b18, 0x0e671b2f, 0x1c9dcf13, 0x0b5966ef, 0x0d3837e2, 0x19b1b9f2, 0x1eb22c7e, 0x1ca27e94, 0x1072f61e, 0x1136f506, 0x0a3b68b7, 0x0f385bf0, 0x06f6e3d0, 0x00947e3b, 0x1e4887bf,
		0x0253af87, 0x014a4773, 0x1b7d41fd, 0x11acd63b, 0x1965db69, 0x1ff4bb9b, 0x0c8e8a69, 0x1fe87169, 0x00817115, 0x1c2a9c24, 0x1d57da96, 0x05cbe183, 0x046de243, 0x16f5bbe0, 0x189947e1, 0x145d22cd, 0x172b7227, 0x097a8755,
		0x0d40c4b5, 0x02192ac3, 0x07905f68, 0x06792b5d, 0x0011aa81, 0x1a9e2250, 0x08bda2ba, 0x0c886a3a, 0x179dc2aa, 0x09e3ff6b, 0x002cc0c9, 0x02882751, 0x161d5e7a, 0x08c27261, 0x1ec1f93e, 0x1e7c21ca, 0x0dcf6fed, 0x0a678317,
		0x12436f9b, 0x05a022f6, 0x0019fe33, 0x0fc54e5c, 0x05872992, 0x0d6b2670, 0x199623d3, 0x035e0b80, 0x17e10241, 0x0979ac02, 0x015530f2, 0x1a11dd73, 0x0f35ff31, 0x0b8fda96, 0x1018ff73, 0x04b8c8ae, 0x12065432, 0x17debba4,
		0x0c9173aa, 0x16aad79e, 0x1abc9419, 0x0af07b36, 0x0ad8d75e, 0x0b691eb3, 0x145599da, 0x1ee3e829, 0x12bc10ac, 0x13a88de7, 0x100748cc, 0x0efe67fc, 0x131f4df9, 0x09277c7e, 0x10d11114, 0x1344d08e, 0x01c14dd8, 0x1d71dff0,
		0x1bc3c1ed, 0x07cb1eaa, 0x0918cc58, 0x1762a1b5, 0x055adefd, 0x14a9cc07, 0x0b920d6e, 0x0e3f08f7, 0x1e438655, 0x0474d78a, 0x195e9a74, 0x1d2fd457, 0x1f5e12f0, 0x1f1581be, 0x18bed1a7, 0x0305619d, 0x1748d430, 0x058d289a,
		0x07ee1062, 0x086c8879, 0x01ca81de, 0x046d74bb, 0x11182eb5, 0x025dd2bd, 0x15ba844f, 0x043c5c8a, 0x131a80fd, 0x04cc6391, 0x1f77deb9, 0x1705a38c, 0x16af9dd7, 0x14f55e70, 0x0bd7b99c, 0x056e7f51, 0x02bb60a4, 0x12cc0542,
		0x0cd61ac1, 0x170c68ae, 0x00797b43, 0x13dd89d5, 0x184bf446, 0x16c79635, 0x085cbec0, 0x01c66e03, 0x0eefd40f, 0x1b4843fd, 0x18d84011, 0x08fe659f, 0x0f4f459b, 0x11ec6c1b, 0x099fd45e, 0x108f35ac, 0x18ea3731, 0x19b52c91,
		0x1a43a033, 0x18d3a5ce, 0x16622aba, 0x0cff2fb8, 0x028f903a, 0x15ed3de0, 0x0fee7558, 0x0bb4a3d8, 0x0a7d5937, 0x183536e6, 0x03d4d62d, 0x03b6c4c0, 0x015bdb59, 0x0b1fca4b, 0x1663f72c, 0x07e88045, 0x18b2aa8a, 0x1c16619d,
		0x15c30417, 0x04cbec6a, 0x12c9fc27, 0x1c6d1f34, 0x0c56e702, 0x1e340af2, 0x0e6c3093, 0x032428f6, 0x0eb9d95c, 0x10758f95, 0x121e27c7, 0x0596b7bb, 0x040a8539, 0x00b7de7d, 0x1fdffaab, 0x107961c9, 0x129efc19, 0x1e3b7e44,
		0x138a76a4, 0x07097201, 0x0a026f00, 0x055cc24d, 0x058a39e8, 0x0df3eacd, 0x12b28254, 0x1993b8a1, 0x15560bf7, 0x063cc02e, 0x1b6474ad, 0x1bf249af, 0x19733a89, 0x11794b21, 0x0cd8ea67, 0x1786fa83, 0x14b63e98, 0x032af682,
		0x0145a42a, 0x14f56119, 0x0a26394f, 0x0ffede11, 0x00e7c8a8, 0x1431d1a2, 0x0e49f3bf, 0x1b299502, 0x1e948c63, 0x0e5a081a, 0x0fc1de06, 0x1afd9bcd, 0x10bdbab2, 0x1c8ee646, 0x0fc09f44, 0x12b4240c, 0x0758406f, 0x0ac39977,
		0x1085c91b, 0x19e1734f, 0x0aa7918b, 0x0d4f1d5b, 0x075502dd, 0x07799ac5, 0x0957b125, 0x0c9fdeb8, 0x0d641ba7, 0x17a3bac9, 0x13403925, 0x14792c2b, 0x1c757bec, 0x136a2143, 0x0c6abecd, 0x069adae2, 0x126704c7, 0x1bdaec44,
		0x03c26dfb, 0x0da4c4e5, 0x0a9bc42e, 0x09e0386e, 0x14754e9f, 0x133202ee, 0x0fc54d9d, 0x1221747a, 0x02b17881, 0x1b94cf0f, 0x03c505f6, 0x06a5b2f0, 0x103a7f99, 0x0431351e, 0x0ace4e13, 0x17f64902, 0x15273fcd, 0x18b52fa9,
		0x063bf194, 0x0316b1fe, 0x10fd5ff3, 0x1f493142, 0x08168e49, 0x1bc630d5, 0x038075c7, 0x1603194f, 0x09396247, 0x12855b93, 0x149ec3a5, 0x03f9a749, 0x0bd537b8, 0x1edf3108, 0x1516799e, 0x09c0279b, 0x1261b66d, 0x05bbc035,
		0x08e14ef9, 0x1dfb651e, 0x0ef9cfba, 0x06afc455, 0x017a153b, 0x1fd1ac05, 0x17992563, 0x194dcaa4, 0x0c2942ad, 0x168e2547, 0x1aa89131, 0x0a8dfb83, 0x137aa1ce, 0x0afc5f35, 0x0ac4a82b, 0x03f6824a, 0x0ca55b70, 0x137c5ade,
		0x0031d59e, 0x0b09e588, 0x036adb46, 0x07fc3ec1, 0x1a9037c0, 0x09c13580, 0x0907d558, 0x0373e12d, 0x17d7ffb7, 0x11d37f92, 0x1b4dcde3, 0x0ba792b0, 0x0efa807d, 0x16199344, 0x1323b634, 0x0c0f3424, 0x150e9658, 0x1b76136b,
		0x1485513e, 0x0313de45, 0x0c7288c3, 0x111cb3b9, 0x1dbadb83, 0x01f949a7, 0x0933e4f9, 0x13bdcb66, 0x13ddebf3, 0x167659ce, 0x15c5ac6e, 0x1f18be8d, 0x1374c514, 0x0089223b, 0x10dc2a6c, 0x0cf4629e, 0x1018cf50, 0x0d5b3e00,
		0x1ddf3c1f, 0x0ce47c79, 0x194821e3, 0x0edf2657, 0x1013bbc5, 0x1b6b7dce, 0x1a0434f7, 0x1fba53f8, 0x0c405b98, 0x1c792c91, 0x12a98f33, 0x1f4419e8, 0x174edb96, 0x11ae5767, 0x10409a95, 0x03601814, 0x0b704a8b, 0x0ebf9615,
		0x0d1e6de3, 0x0b744d65, 0x0ed5a1ad, 0x1924cc42, 0x1c6e9e56, 0x0ccc7e92, 0x173d90cb, 0x0f07d97d, 0x1a94711c, 0x0885e264, 0x1e08a682, 0x1765a180, 0x0789c48f, 0x0e3445ac, 0x1670ce10, 0x0d8b06e8, 0x1f17e32f, 0x16b1ff3d,
		0x1c10f154, 0x0fb2b840, 0x0eabd40a, 0x085ab463, 0x17290de9, 0x159077ca, 0x1daac827, 0x0ea08590, 0x1cf8f5cc, 0x07f9bee0, 0x0956745a, 0x1349e8b8, 0x0375fd2f, 0x0c54008b, 0x0304dca2, 0x1f192207, 0x0dc6345e, 0x15d70886,
		0x06f85eea, 0x03f19a03, 0x0da5f3af, 0x12747f09, 0x0de5f140, 0x16ff88b9, 0x1764404c, 0x16bf3f2c, 0x02d3f63a, 0x1cb5555c, 0x1bedbb5d, 0x0595cbed, 0x032b9f9d, 0x07f3e28d, 0x02af6b1e, 0x0d303b4d, 0x14470cbd, 0x1c0c3925,
		0x1189cd12, 0x0db78de6, 0x0d57eaf0, 0x06cddef4, 0x0ed497cc, 0x1ad5e5df, 0x11f2db43, 0x025ad108, 0x06f7707a, 0x07e186a6, 0x06f4a69c, 0x0336cf60, 0x1a2269bd, 0x090e5bdd, 0x013667c4, 0x0ff18139, 0x0f2b4a08, 0x10c34763,
		0x1d339408, 0x1c29da79, 0x01fd7a82, 0x0e063dbb, 0x08822cc9, 0x088ba25d, 0x1d18171b, 0x0d4e24f0, 0x1fce86ae, 0x1e26a203, 0x04a89f51, 0x06a4b346, 0x18ad3b6f, 0x07ca3c63, 0x12eadc51, 0x182ee2c1, 0x0be2b3ee, 0x0cf83dac,
		0x0a7c9d2c, 0x08218e8c, 0x11f92c35, 0x0c7038cf, 0x1a850665, 0x071fc08a, 0x1eba1ee2, 0x1ca44033, 0x0e8356a9, 0x1da1ea90, 0x1db6402a, 0x07d0adf7, 0x1d852947, 0x0a57f287, 0x0f2f24d0, 0x1f780d47, 0x1e0ffa7e, 0x15182449,
		0x16251506, 0x188bd48f, 0x04560701, 0x09fa7bc1, 0x073b0023, 0x0b11e53b, 0x1a5001bb, 0x0b975a31, 0x1e490eb4, 0x13ae2bf9, 0x1f9787e7, 0x0cbe9bd9, 0x17ec8bec, 0x12557656, 0x19020cae, 0x0267e960, 0x0534c5da, 0x1f8a8c5b,
		0x19f14bfc, 0x02e1c9eb, 0x13cfa1f3, 0x00e25658, 0x0189c76d, 0x05d9477c, 0x02682816, 0x14fef1e3, 0x1a610938, 0x09233351, 0x08cb7207, 0x0b9e3830, 0x0716feb5, 0x0b05ff92, 0x135769c9, 0x15da4d7b, 0x0d2eb7a5, 0x16c7f772,
		0x1c256819, 0x130e4e0b, 0x1250c672, 0x19c7c500, 0x01d392f1, 0x03a16113, 0x1a2254a5, 0x0657aea5, 0x128a792d, 0x18bad5df, 0x1c55b556, 0x08bf2b8d, 0x08dfe88f, 0x0de23d79, 0x1a09adfc, 0x15d44a61, 0x082b3d78, 0x044cef8c,
		0x1e5a2070, 0x0a699a77, 0x043022d6, 0x1c133ae0, 0x03cc2813, 0x00290321, 0x1cde91c8, 0x1df2b9ad, 0x087b29ac, 0x006740bb, 0x18c8c874, 0x1777c030, 0x0d6fe687, 0x06873ec5, 0x11c2b071, 0x0f1c4e5e, 0x03e6f9b2, 0x0095ffd5,
		0x1645c26c, 0x1ce518f3, 0x06be8ddf, 0x08d819e3, 0x0a85c54b, 0x08a8088a, 0x08e782f4, 0x1c8475a2, 0x0e388e7d, 0x1e07ebd2, 0x0c299905, 0x1e0b9d00, 0x049b816d, 0x1fa3ee05, 0x05c7cd9e, 0x11da73de, 0x048da0f3, 0x1691489c,
		0x09eea69d, 0x0d859573, 0x126fe435, 0x1c6b74d4, 0x0e2d23cf, 0x0f94c88c, 0x160d8d91, 0x18186479, 0x00fc649a, 0x1382b044, 0x0afffb9f, 0x1af8fbca, 0x03d98dc7, 0x16b6b5bf, 0x05b710df, 0x182c12cf, 0x1e70464f, 0x1aa1fc10,
		0x1e38d7b9, 0x0537b67f, 0x1a41dc14, 0x110fa182, 0x18f62500, 0x0461e619, 0x08497706, 0x131e80ce, 0x07b52553, 0x11a3c82a, 0x06d228cd, 0x1952352d, 0x1ac6f566, 0x0879746d, 0x113fb188, 0x09335d88, 0x07b80a4c, 0x134c6d96,
		0x03c76485, 0x108465c2, 0x1315e6a2, 0x1cfd23c0, 0x19552d30, 0x088db223, 0x1743ed01, 0x18d2b227, 0x00d57478, 0x0c02d637, 0x0d95413a, 0x047dda80, 0x1f9f662d, 0x13ef30dd, 0x0af4995b, 0x1f4cc208, 0x1a9cb8cf, 0x08dc9876,
		0x01a14c54, 0x157e5bef, 0x0f55d303, 0x1fd8903a, 0x127b64e3, 0x13df467e, 0x07911a57, 0x0a2f3e21, 0x05cac964, 0x19244cd0, 0x19703c3c, 0x1a4c2d97, 0x02f164ec, 0x0bbdc0b0, 0x0a03947b, 0x07530421, 0x147a1614, 0x0ca21939,
		0x023a15ac, 0x1c4f7f47, 0x132cf3f6, 0x08598315, 0x11d898d8, 0x16eb276b, 0x0287bcd5, 0x0e0b8197, 0x0d1f907e, 0x0883ef0f, 0x054fe1a1, 0x1fb27b23, 0x1eca936f, 0x06c74dc3, 0x175c7fbe, 0x14e9758e, 0x1cb0ab44, 0x1aecb7d7,
		0x1f86470f, 0x1eb9a38d, 0x0ef03eb4, 0x1b2839d5, 0x1b2e1081, 0x0da475e7, 0x17101430, 0x1123cdb2, 0x0db4503d, 0x0512d1c5, 0x0fc38c80, 0x005e7e3b, 0x19609820, 0x08a51d37, 0x0200ddd1, 0x0ae7be17, 0x0fa01bd0, 0x1250700e,
		0x09a0e2a4, 0x1f43382c, 0x1333b9bf, 0x1f814b36, 0x03049f29, 0x086c779e, 0x0267e966, 0x1364918f, 0x19584978, 0x0abcae00, 0x06bde0c4, 0x0e46bba4, 0x1ba72a1d, 0x0a5025d5, 0x03302f7e, 0x0667d1e4, 0x0a77c8b8, 0x1197e50d,
		0x07b8c274, 0x13045659, 0x1d03d44e, 0x14dda678, 0x04244eab, 0x06666b06, 0x0552a73a, 0x19611d92, 0x194e80b0, 0x1d64171b, 0x0dbad7d4, 0x09440694, 0x07e37bad, 0x0e74942d, 0x0bbaae88, 0x16b8ff4e, 0x0747db22, 0x05454812,
		0x009aeec8, 0x1590cc83, 0x1f6180b4, 0x068d3c60, 0x1c4a2a56, 0x0457ea71, 0x06439424, 0x1f4bd73a, 0x157af8b9, 0x1b9e6ed8, 0x017ec690, 0x1ae86920, 0x1d06b4d8, 0x05caf7aa, 0x151e4c57, 0x1ea65160, 0x16399104, 0x195d4694,
		0x07b93a32, 0x02e7af3b, 0x152a6713, 0x1ea463a6, 0x1eb4e7a8, 0x019074d3, 0x1d9b614b, 0x1d590f8a, 0x11028076, 0x1dca85f4, 0x127dff03, 0x04cec30e, 0x140c9251, 0x0eec53a7, 0x1332989a, 0x02a5eb6f, 0x0163d408, 0x02ef6108,
		0x195d0a6b, 0x0254396c, 0x1542daba, 0x132b53de, 0x0e22a48a, 0x0bea7b6a, 0x11b7fee7, 0x18a47a23, 0x057c7c31, 0x0b5e2f18, 0x1efd1266, 0x17811b92, 0x1c1b2947, 0x1be49a7f, 0x096e859e, 0x1724fa08, 0x1dc087a9, 0x02f4c52b,
		0x1ebd1c3b, 0x03aafa27, 0x16658605, 0x09326ebe, 0x0c174e3e, 0x0d2bda0c, 0x10563f95, 0x09db61f8, 0x0e9f374e, 0x017cb4fc, 0x09a6fadd, 0x0d94f908, 0x08503bb9, 0x1d64f97a, 0x06b0e832, 0x1b18d95f, 0x02c32097, 0x13b83ab9,
		0x08d86804, 0x09928ed2, 0x074eb2c7, 0x142e166f, 0x1adc4e80, 0x0d7ac3cf, 0x1877f67a, 0x0125b8a9, 0x02f641dc, 0x0e066782, 0x1bb0d1b9, 0x04d52a48, 0x02cf573a, 0x02b69afe, 0x15e95ef8, 0x0e018b76, 0x0f5b7acd, 0x0e68fd10,
		0x0143f40e, 0x141bcd36, 0x0fb53b6c, 0x10f955b5, 0x02da68b5, 0x09b24e70, 0x11c4613a, 0x0ace1585, 0x1c033a34, 0x09ed987e, 0x1edb834b, 0x0a7c2f7f, 0x0e790eec, 0x12d75ad6, 0x0e289101, 0x1bbd340a, 0x0a0748fd, 0x0444fe3a,
		0x187bd7d3, 0x1c6f4a7d, 0x198838d2, 0x1a701e46, 0x11361229, 0x1ae6ddcf, 0x12a5bf7b, 0x08b365ff, 0x16549592, 0x0b6cc6f4, 0x05c3f332, 0x04c22470, 0x1778f29f, 0x0663672d, 0x1cdfbe39, 0x096e08d4, 0x03c26dfb, 0x03bd4986
	};

global constexpr u32 MAX_RAND_TABLE_VALUE =
	[]()
	{
		u32 n = 0;
		FOR_ELEMS(it, RAND_TABLE)
		{
			if (*it > n)
			{
				n = *it;
			}
		}
		return n;
	}();

internal f32 rng(u32 seed)
{
	return static_cast<f32>(static_cast<f64>(RAND_TABLE[(seed + RAND_TABLE[seed % ARRAY_CAPACITY(RAND_TABLE)]) % ARRAY_CAPACITY(RAND_TABLE)]) / MAX_RAND_TABLE_VALUE);
}

internal f32 rng(u32* seed)
{
	return static_cast<f32>(static_cast<f64>(RAND_TABLE[(++*seed) % ARRAY_CAPACITY(RAND_TABLE)]) / MAX_RAND_TABLE_VALUE);
}

internal i32 rng(u32* seed, i32 start, i32 end)
{
	return static_cast<i32>(rng(seed) * (end - start) + start);
}

internal f32 rng(u32* seed, f32 start, f32 end)
{
	return rng(seed) * (end - start) + start;
}
