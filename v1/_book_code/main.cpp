#include <iostream>
#include <string>

using namespace std;

#include <openssl/sha.h>

string sha256(const string &str) {
    char buf[3];
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, str.c_str(), str.size());
    SHA256_Final(hash, &sha256);
    std::string newString;
    for (unsigned char i : hash) {
        sprintf(buf, "%02x", i);
        newString.append(buf);
    }
    return newString;
}

int main() {
    std::string x = "hello";
    cout << sha256(x) << endl;
    return 0;
}