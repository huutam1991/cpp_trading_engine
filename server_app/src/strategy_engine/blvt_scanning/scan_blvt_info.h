#ifndef SCAN_BLVT_INFO_H
#define SCAN_BLVT_INFO_H

#include <memory>
#include <unordered_map>
#include <mutex>

#include <util_macros.h>

class Json;
using namespace std;

class ScanBLVTInfo
{
    Singleton(ScanBLVTInfo)

public:
    void subscribe_symbol(const string& symbol);
    void unsubscribe_symbol(const string& symbol);

    void test_change_info(const string& symbol, bool is_baskets_changed);

private:    
    unordered_map<string, long> m_blvt_map;
    unordered_map<string, long double> m_baskets_map;
    unordered_map<string, long double> m_token_issued_map;
};

#endif //SCAN_BLVT_INFO_H
