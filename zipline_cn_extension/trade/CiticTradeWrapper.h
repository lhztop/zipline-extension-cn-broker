#ifndef CITIC_TRADEWRAPPER_H
#define CITIC_TRADEWRAPPER_H

#include <string>
#include "CITICs_HsT2Hlp.h"
#include "json/json.h"

#pragma warning( disable : 4507 4514 4820 4710 4996)
class CiticTradeWrapper
{
private:
	char fund_account[32] = { 0 };//资金账号
	char password[32] = { 0 };//资金密码
	HSHLPHANDLE HlpHandle = NULL;

	char op_entrust_way[4] = { 0 };//外部接入客户必须用3（远程委托），生产后需要让营业部对使用的资金账号开通3权限
	char ClientName[16] = { 0 }; //客户简称

	char client_id[32] = { 0 };
	char user_token[64] = { 0 };
	char branch_no[8] = { 0 };
	char asset_prop[4] = { 0 };
	char sysnode_id[4] = { 0 };
	char entrust_status[8] = { 0 };
	char op_station[1024] = { 0 };
	char stock_account[128] = { 0 };
	char IP[16] = { 0 };
	char MAC[16] = { 0 };
	char HD[16] = { 0 };
	bool connected = false;
	Json::FastWriter _jsonWriter;

protected:
	void SetNecessaryParam(HSHLPHANDLE HlpHandle);
	bool GetLocalIP(char* ip);
	bool GetFirstMac(char * mac);
	bool connectServer();


public:
	char ConfigFile[1024] = { 0 };
	bool Login();
	std::string System();
	std::string Portfolio();
	std::string Positions();
	std::string Order(const char* stock_code, float price, int volume);
	std::string Transactions();
	std::string Orders();
	std::string CancelOrder(const char* orderId);

	CiticTradeWrapper(const char* account, const char* password, const char * configFile = "Hsconfig.ini", const char* op_entrust_way = "3", const char* clientName = "ZZJJ");
	~CiticTradeWrapper();
	
};

#endif
