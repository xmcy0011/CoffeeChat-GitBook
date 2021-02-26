# Secret Chat Encryption

本章探讨在不安全的网络下（路由器抓包、运营商注入广告等中间人监听方式）如何对聊天内容进行加密，即在网络上传输的都是密文，只有知道密钥的双方才能解密出内容。

## 基础

### 对称加密和非对称加密

对称加密：AES算法
非对称加密：RSA算法

AES加密的代码示例：
```c++
int AES_set_encrypt_key(const unsigned char *userKey, const int bits,
                        AES_KEY *key);
int AES_set_decrypt_key(const unsigned char *userKey, const int bits,
                        AES_KEY *key);

void AES_encrypt(const unsigned char *in, unsigned char *out,const AES_KEY *key);
void AES_decrypt(const unsigned char *in, unsigned char *out,const AES_KEY *key);
```

```c++
// 设置密钥 AES key
std::string aesKey="12345678901234567890123456789012";
AES_set_encrypt_key((const unsigned char *) strKey.c_str(), 256, &m_cEncKey);

int CAes::Encrypt(const char *pInData, uint32_t nInLen, char **ppOutData, uint32_t &nOutLen) {
    if (pInData == NULL || nInLen <= 0) {
        return -1;
    }
    uint32_t nRemain = nInLen % 16;
    uint32_t nBlocks = (nInLen + 15) / 16;

    if (nRemain > 12 || nRemain == 0) {
        nBlocks += 1;
    }
    uint32_t nEncryptLen = nBlocks * 16;

    unsigned char *pData = (unsigned char *) calloc(nEncryptLen, 1);
    memcpy(pData, pInData, nInLen);
    unsigned char *pEncData = (unsigned char *) malloc(nEncryptLen);

    CByteStream::WriteUint32((pData + nEncryptLen - 4), nInLen);
    for (uint32_t i = 0; i < nBlocks; i++) {
        AES_encrypt(pData + i * 16, pEncData + i * 16, &m_cEncKey);
    }

    free(pData);
    string strEnc((char *) pEncData, nEncryptLen);
    free(pEncData);
    string strDec = base64_encode(strEnc);
    nOutLen = (uint32_t) strDec.length();

    char *pTmp = (char *) malloc(nOutLen + 1);
    memcpy(pTmp, strDec.c_str(), nOutLen);
    pTmp[nOutLen] = 0;
    *ppOutData = pTmp;
    return 0;
}
```

### 主流加密方式：微信和Telegram是如何对内容进行加密的？

### 思考：

## 进阶：Telegram 的 P2P 加密

### Diffie-Hellman key exchange

to do...

see:

- [telegram: MTProto Mobile Protocol](https://core.telegram.org/mtproto)
- [telegram: Security Guidelines for Client Developers](https://core.telegram.org/mtproto/security_guidelines)
