#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "internal.h"
#include "types.h"

static const u64 crc64_tab[256] = {
    0x0000000000000000ULL, 0xd1c5da906bb178e8ULL, 0xac3990239075600cULL,
    0x87ce8b48c746baa2ULL, 0x38a2363a353e031cULL, 0xc46359edb7b47e7aULL,
    0xa4e568b562ee63a3ULL, 0x9d915b3705a08131ULL, 0x58d9d51ccb1746c0ULL,
    0x39944168dce4519cULL, 0x92651993bebe44caULL, 0x7a42477977da84a1ULL,
    0x34db1c3e71c89de2ULL, 0x74625d66da7b2961ULL, 0x6909825cd1905142ULL,
    0x07d3329498359d3dULL, 0xe8445114a920053aULL, 0xc1db15c4b903d421ULL,
    0xed14632ab2d06634ULL, 0x70b95c9d7485253eULL, 0x62d0a04a724b30aaULL,
    0x26015340623d0973ULL, 0x14b92a3b0a65b364ULL, 0x432d84d3425378a0ULL,
    0x646e05bd28d93360ULL, 0xe4906e55d7a67086ULL, 0xc65945d8932e6ee9ULL,
    0xbdede7e00c75189eULL, 0xba9e0b9b6638cea7ULL, 0xb9d39425ae21db45ULL,
    0xd2b24e25abcc3b70ULL, 0x82b88710da92be63ULL, 0x5b78710423ca5653ULL,
    0x37a695a4c8900d54ULL, 0x9b955c10633e7b62ULL, 0x4cbbda1728993768ULL,
    0x8aa806b8d0824179ULL, 0x00387099cc0c03edULL, 0x027c9471dc57d2b3ULL,
    0x06a5b4cb4ce0cb69ULL, 0x5bbced2676862d73ULL, 0xc6485cdbe0931d0dULL,
    0x33439138d3c294a5ULL, 0x217889b981b37cedULL, 0x528330260c1405b4ULL,
    0xc9bb478968a0a7b4ULL, 0x2230c2a669c4ad06ULL, 0x266b8ec16b8e5338ULL,
    0xebe28b9479008a54ULL, 0x93237bb021d41d41ULL, 0x32ebd3ed346d8705ULL,
    0x23e1bcca7cc9a93cULL, 0x1c3b2eb0d605e592ULL, 0xb79179d798042590ULL,
    0xae666314a3723e70ULL, 0x54c09a36d927eb9eULL, 0xbda8a1aca720028cULL,
    0x03345d839acea949ULL, 0x4d2b565b2b1ebea7ULL, 0x34e7419d924119aeULL,
    0xdd12176c7302397aULL, 0x4c87142a0c701e7eULL, 0x59a9deaa7272978eULL,
    0x28c0b934408bc594ULL, 0x7bb7a6bb057a8a10ULL, 0x3a89d4b7de08c67cULL,
    0xc0386d958641c9e8ULL, 0x11035396ccb362bcULL, 0x9688748aac668b82ULL,
    0x9bb2743daaba166aULL, 0xd0c2ccabe7b3dcd7ULL, 0x5025de0e9b316da5ULL,
    0x7cedae708ccee884ULL, 0x8bc1a8e309e18905ULL, 0x6be60c66b22ec46aULL,
    0xb7c127005e773656ULL, 0x8d469259d5149d02ULL, 0x57c77e40297ce432ULL,
    0x454445905cc8c01eULL, 0x7176000aa9a3c604ULL, 0xbcd47ec15c71bb93ULL,
    0xe585191a5703ca5bULL, 0x98706aa6e521578cULL, 0xd8e37b54a58aa66aULL,
    0xed84ed283dd3a73eULL, 0x1aed4276e140d1b3ULL, 0x306509ec87da33b7ULL,
    0xed67bbe69a25d2ccULL, 0x3139238c7a3cede3ULL, 0x9aa81385ad4292d1ULL,
    0x4682bd4d622be761ULL, 0xce69be45e3749c90ULL, 0xe72139e2824c08d7ULL,
    0x969d5606354d854cULL, 0xb3d8b648c898e06dULL, 0xd259dbb41ba03ec9ULL,
    0xbed8287759ca9154ULL, 0x953eb6315a3785c2ULL, 0xbd42683d8ddae9c1ULL,
    0x3c6598cc3d7a3766ULL, 0xda02e1ce68193c8eULL, 0x330aa3b4e31ba303ULL,
    0xb39a5216cdec7536ULL, 0x3950900acd2a8059ULL, 0xda369cca5d73b31dULL,
    0xd5c63dc189458d78ULL, 0x9d47d66274eae478ULL, 0xe46c228e655a2db8ULL,
    0x0529d3252ae52a98ULL, 0x4634724db8e34a73ULL, 0x4e16782694451db9ULL,
    0x00275b2e5b700e8cULL, 0xb99864a3d48d980cULL, 0x5048cac029a89929ULL,
    0xb3bd3ad89d63cc7dULL, 0xa3c481083c8e1da7ULL, 0xa1ed5b960d6e3731ULL,
    0x89d8c806b3adc8e3ULL, 0xe1591d0d8ead2937ULL, 0x2437992a3bade7e9ULL,
    0x774b3bc10913935aULL, 0x264a154adabb5e46ULL, 0x9ad811546278de69ULL,
    0x1bb4cb5ed028c179ULL, 0x5c4615cb869499b8ULL, 0xd124ca02b26cb687ULL,
    0xd6374e8a0c9c8d1eULL, 0xb3686e3cbead2eacULL, 0x2cca5e9133754de5ULL,
    0xe9975c8085eee126ULL, 0x1b6d55a51398c85dULL, 0xe6ba7710bac6342aULL,
    0x4908b2c0be232bb0ULL, 0x2bbdb8b937c295edULL, 0x624c626cad91c279ULL,
    0xbe82e4883d93a6e1ULL, 0x7a13d6dd6deb29c4ULL, 0x611272069372b2a1ULL,
    0xd55299b92e592edeULL, 0x9c2cb711e4184c05ULL, 0xe21752239daab3bdULL,
    0x82b8b398abca56caULL, 0xb3208678b3cd6905ULL, 0xae93381879d05d8aULL,
    0x64146a06734b1cecULL, 0xae94c5083d4104c4ULL, 0xe79c656eab152019ULL,
    0x96c7aae6828807e7ULL, 0x7c46c6329367468aULL, 0x8ae88a8ab5d68c7dULL,
    0x0e31d39e0dc1734dULL, 0xb02ab2737ed542beULL, 0x9552acbe78da51d6ULL,
    0xb7ce54333729e760ULL, 0xc2eb14d2d209054bULL, 0x912ad5d0a8354dc6ULL,
    0x23a13d0595501942ULL, 0xa1b47e267c470a21ULL, 0x3ea7bc8223e5c741ULL,
    0x73471a65b1833cd3ULL, 0x2d16e622daa16267ULL, 0x71c49d5c87294198ULL,
    0x69546db5ba9bad4eULL, 0x6b8dc1669986812cULL, 0xd5c79347d5784944ULL,
    0x4e44eeedee9a9630ULL, 0xc0456b680d2bcb42ULL, 0x7e9ec89b2eeaa1c9ULL,
    0xd82d9cb2d12abab6ULL, 0x2302e3429509bb44ULL, 0xae306738e5aada9bULL,
    0xb79179d755025986ULL, 0xb79179d755086986ULL, 0xe0b0d0b07a2a5eb4ULL,
    0x1924d98e2be0e363ULL, 0x4b0ed8b3e33253d1ULL, 0xaba6960160d2d788ULL,
    0xce21a08de8c2a2eaULL, 0x82991d33db44258eULL, 0xea5784d861427a90ULL,
    0x55b80a0ec4b5b65aULL, 0x1c15e7d46146e996ULL, 0x42038ca2cdd715e5ULL,
    0xe51d3bb7e4ce5e94ULL, 0x81082245671a8749ULL, 0x24d079115a594c88ULL,
    0xd12834cc6898b7b0ULL, 0x2086528cdd7eb50dULL, 0x49abe865c4560b16ULL,
    0x5ae0135251bc4e93ULL, 0xed3bd31beb06131bULL, 0xbedd7d03349b0263ULL,
    0x65a5e3a24cce1d7dULL, 0x3eee6317cbb523a3ULL, 0xe51302892558bb4aULL,
    0x37828dce30dd0244ULL, 0x356a1ebc54d39e16ULL, 0x3c98c633240c45eeULL,
    0x1158c46cedd95534ULL, 0x075cdbccab258309ULL, 0x90be6be316dd55acULL,
    0x820078ad91440d9dULL, 0xdc10d77142d403a1ULL, 0x32ad7663ec617ba9ULL,
    0xc70e226cc0ae4eb9ULL, 0x33cc4605b8c6048eULL, 0xc6dad027b5d0cd9dULL,
    0x944b343ddbc8b6abULL, 0xb7257761c497116eULL, 0x87ec08b8923ea961ULL,
    0x233e7b46d87eb0edULL, 0xa7d3ab40b763a7ebULL, 0x4a07b6deac878369ULL,
    0x7eb99d42dd3b77c1ULL, 0xac847e816d3eeeecULL, 0xe38d3c0288042cc8ULL,
    0x8e58c24294037e2eULL, 0xe945016913ae13d3ULL, 0x0499305348705db0ULL,
    0xd512268ddc279279ULL, 0x5ab4d4b554ae93daULL, 0x9d23e167e3748be7ULL,
    0x1d7691e190801028ULL, 0x093cb504aabc1b07ULL, 0xb4ad34e87a466230ULL,
    0xac612b9b02915d19ULL, 0xb1cc14276e8e63caULL, 0xa20e918a629e36c5ULL,
    0x34d2e350ac599444ULL, 0x854d36735d825b11ULL, 0xdae22788c368cd41ULL,
    0xa128989097a0673dULL, 0x2269bc223a977688ULL, 0x562334bd77eb112aULL,
    0xeb9c8e03b71572a9ULL, 0x83a06d8ce0994c1bULL, 0xbd6e8eae90d601c2ULL,
    0x0700981e0648ad49ULL, 0x0ea9c3a95e55d49eULL, 0x276b22ad86bc25a3ULL,
    0x974bae0042eac365ULL, 0x5e404a42a3803565ULL, 0x74a744bbe8c2c5adULL,
    0xb277370720e8a0a0ULL, 0xd5c2a54936412492ULL, 0x6641e5594b4696bdULL,
    0x2155520694c1d12aULL, 0x360e151ebda39105ULL, 0x085db9c3beeaed22ULL,
    0x4cde8122b75d2c1eULL, 0x9a4c65a9570eaa41ULL, 0x27026e430867b12eULL,
    0x350c7c1da2933536ULL, 0x7948ae282ea053daULL, 0xb09391c11eb10caaULL,
    0xa43b4c69ed853a52ULL,
};

/**
 *@brief 64位校验
 *
 * @param s
 * @param l
 * @return u64
 */
u64 crc64(const char *s, int l) {
  int j = 0;
  u64 crc = 0;

  for (j = 0; j < l; j++) {
    u8 byte = s[j];
    crc = crc64_tab[(u8)crc ^ byte] ^ (crc >> 8);
  }
  return crc;
}

static unsigned long crc32_tab[] = {
    0x00000000L, 0x22b8b53eL, 0x563e1597L, 0x7e9b4481L, 0x6ace6073L,
    0x244e1e9aL, 0xd970b4b8L, 0x7540103L,  0x0a392bdbL, 0x79820ecaL,
    0xeac07a4bL, 0x5315b8b9L, 0x1b7cb2ceL, 0xdce2781aL, 0x30ada1d9L,
    0xa185b334L, 0xaa397374L, 0x1ebc7cb5L, 0xda8ad740L, 0xd32b029aL,
    0x25e6a405L, 0x26ce7d0bL, 0xb09d88ccL, 0x21e31256L, 0x892a59c3L,
    0x4456ee55L, 0xd92d991cL, 0x66dd98a6L, 0xb9556684L, 0x545c9937L,
    0x26ed6802L, 0xa302b353L, 0x4c1dc103L, 0x7bc3d013L, 0xa3a34beaL,
    0x1e7e0311L, 0xbe68d5ecL, 0x885ab038L, 0x7da02a53L, 0x168d0e09L,
    0x6be3d49aL, 0x89bb9162L, 0x5c4db3b8L, 0x55bda554L, 0xa699e434L,
    0x455e5d24L, 0x8cbe2247L, 0x43e14d69L, 0x05a22896L, 0x18521dbbL,
    0x51895509L, 0x7c3073a0L, 0x85b74c6dL, 0x67d23b1eL, 0xa7aace9dL,
    0x4442c9cdL, 0xb5554a0dL, 0xcc775135L, 0x68ac3b18L, 0x143e6b6dL,
    0x36603b47L, 0xb2bb5eb4L, 0x9e328325L, 0x834c3bc7L, 0x03eb4196L,
    0x8e4262eaL, 0x71d6e395L, 0x7c1872c3L, 0x7749977L,  0xde3a0160L,
    0x3bdb6da8L, 0x1a0b26ceL, 0x029dd1ebL, 0x7026d175L, 0x3de099baL,
    0x5c202cd8L, 0x4d851b38L, 0x4e5da262L, 0x71137d7cL, 0xd5b12344L,
    0x89d20d56L, 0xa54ccd7dL, 0x47634121L, 0x259b7abbL, 0xee1cdcdeL,
    0xa4927dabL, 0x563c5552L, 0x32509cbcL, 0x7bd7222bL, 0x9aa38414L,
    0x0954cd91L, 0xa9d78083L, 0xca66c73eL, 0x7cb066d9L, 0x42359515L,
    0x8807ed17L, 0xe57ede72L, 0x8eccc540L, 0x10eecbedL, 0x24385d60L,
    0x6744bb39L, 0x603d90d7L, 0x7dd7559eL, 0xcc8537e4L, 0xb9ebc2edL,
    0xb9cb2e51L, 0x87ec5142L, 0x909ad5c8L, 0x4c7e4dcaL, 0x8e5a03a5L,
    0x6c92408bL, 0xd2bc96d6L, 0x03e4c010L, 0x96c8b389L, 0xd3e6ee11L,
    0x3e249b39L, 0x9dc8ae10L, 0xc7028880L, 0xeeb4b30dL, 0x95391ea9L,
    0x2b78e422L, 0xda40eb75L, 0xd94e778bL, 0x85693639L, 0x9ab72711L,
    0x35ca1a10L, 0xee0c596eL, 0xc9127eb4L, 0xe804b143L, 0x2912de96L,
    0x8796ec35L, 0x73a3b8c9L, 0x2548eebeL, 0x3978ba17L, 0xa075c205L,
    0xd1db2b87L, 0x598e97e6L, 0x70463c17L, 0x447c7cdaL, 0x81cc848bL,
    0x27749db8L, 0x4b3c2840L, 0x1c2507e2L, 0x100a2e3bL, 0xeb188a9dL,
    0x19bdaa68L, 0x648dc8a6L, 0xb4eb86d7L, 0x5b78a416L, 0xac01c537L,
    0xc93b3d70L, 0xca889271L, 0x31a04a57L, 0xac3aa6d0L, 0x893235bdL,
    0x3163c414L, 0x095153aaL, 0x4a55dd04L, 0x385b99b5L, 0xbec81b00L,
    0x3e722d83L, 0x3a1891e0L, 0xd9491932L, 0x67944a8aL, 0x95b2cb40L,
    0xcdc32651L, 0x0721a1daL, 0x1c65ca32L, 0x874ded28L, 0x8c2c62ebL,
    0x317763L,   0xdee8b700L, 0xabc86e21L, 0xa17d3e2eL, 0xc51dc849L,
    0xa027318bL, 0xb4cb602bL, 0x7b520594L, 0x1cc6cc2aL, 0xda0721d2L,
    0x4b106950L, 0x714795cbL, 0x17321388L, 0x0657d6deL, 0xcd48a123L,
    0x4469e565L, 0xbeca3159L, 0xd1e5a36bL, 0x29a5b67bL, 0x2cca241dL,
    0xb4a16d70L, 0xba78e81aL, 0x664e9567L, 0xa4c49eb2L, 0x50b2cdc7L,
    0x4d8e3007L, 0xe294740aL, 0x8666c014L, 0xda57e7beL, 0x123c9ea7L,
    0x02a0e0b9L, 0x99b44ed2L, 0x1e8d7b07L, 0x1bec53a3L, 0xca002003L,
    0x13a3e813L, 0xd11c05c9L, 0xdad98230L, 0x8b61b0ebL, 0x3a0ce21dL,
    0x43cb5967L, 0xd7e98919L, 0x82345a39L, 0xa0ce406bL, 0x5569aa8cL,
    0x7dd63457L, 0xd891632eL, 0xdd76c8e0L, 0x32465964L, 0xecb6a1c1L,
    0xb8850429L, 0x3a810468L, 0x87959abeL, 0x61a683b5L, 0x557aaa35L,
    0xd9ed6be3L, 0xee8ab237L, 0x1eee39eaL, 0x35d359d2L, 0xe9a8d518L,
    0x9e5d193eL, 0xd20b6b12L, 0xa0c348baL, 0x457b6de1L, 0x120d3da4L,
    0x744924a8L, 0x991d9840L, 0x6ed53ac7L, 0x99073a28L, 0x0cdc9a5aL,
    0x9939d57eL, 0x0b60ce67L, 0xcdc67080L, 0x8401d5b7L, 0x9d24e838L,
    0x4258e2d8L, 0xa69c22e1L, 0x758b0b42L, 0x53354752L, 0x066ac623L,
    0x58194cbeL, 0x94853db8L, 0xacb43c87L, 0x663a25d9L, 0xa974485dL,
    0xb299e913L,
};

/**
 *@brief 32位校验
 *
 * @param s
 * @param len
 * @return unsigned long
 */
unsigned long crc32(const unsigned char *s, unsigned int len) {
  unsigned int i;
  unsigned long crc32val;

  crc32val = 0;
  for (i = 0; i < len; i++) {
    crc32val = crc32_tab[(crc32val ^ s[i]) & 0xff] ^ (crc32val >> 8);
  }
  return crc32val;
}

s32 hashmap_hash_s32(const char *keystring) {
  u32 key = crc32((unsigned char *)(keystring), strlen(keystring));

  key += (key << 12);
  key ^= (key >> 22);
  key += (key << 4);
  key ^= (key >> 9);
  key += (key << 10);
  key ^= (key >> 2);
  key += (key << 7);
  key ^= (key >> 12);

  key = (key >> 3) * 6364247124;

  return key & HASHMAP_MASK;
}