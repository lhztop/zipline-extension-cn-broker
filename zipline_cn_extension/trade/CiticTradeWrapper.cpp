#include "CiticTradeWrapper.h"
#include "CITICs_HsT2Hlp.h"
#include <stdio.h>
#include <sys/types.h>
#include <assert.h>
#include "json/json.h"
#include <string>

#ifdef _WIN32
    #include <Windows.h>
    #include <Wincon.h>
    #include <nb30.h>
    #pragma comment(lib,"Netapi32.lib")
    #pragma comment(lib, "CITICs_HsT2Hlp.lib")
    #pragma comment(lib,"wsock32.lib")
    #pragma comment(lib,"CITICs_HsT2Hlp.lib")
#else
    #include <ifaddrs.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
#endif


void CiticTradeWrapper::SetNecessaryParam(HSHLPHANDLE HlpHandle)
{
    CITICs_HsHlp_BeginParam(HlpHandle);
    CITICs_HsHlp_SetValue(HlpHandle, "client_id", this->client_id);
    CITICs_HsHlp_SetValue(HlpHandle, "fund_account", this->fund_account);
    CITICs_HsHlp_SetValue(HlpHandle, "sysnode_id", this->sysnode_id);
    CITICs_HsHlp_SetValue(HlpHandle, "identity_type", "2");
    CITICs_HsHlp_SetValue(HlpHandle, "op_branch_no", this->branch_no);
    CITICs_HsHlp_SetValue(HlpHandle, "branch_no", this->branch_no);
    CITICs_HsHlp_SetValue(HlpHandle, "op_station", this->op_station);
    CITICs_HsHlp_SetValue(HlpHandle, "op_entrust_way", this->op_entrust_way);
    CITICs_HsHlp_SetValue(HlpHandle, "password_type", "2");
    CITICs_HsHlp_SetValue(HlpHandle, "password", this->password);
    CITICs_HsHlp_SetValue(HlpHandle, "asset_prop", this->asset_prop);
    CITICs_HsHlp_SetValue(HlpHandle, "user_token", this->user_token);
    CITICs_HsHlp_SetValue(HlpHandle, "request_num", "800");//这个值决定了最大的返回条数，如果不送，则默认为50
}

#ifdef WIN32
bool CiticTradeWrapper::GetLocalIP(char* ip)
{
    //1.初始化wsa
    WSADATA wsaData;
    int ret = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (ret != 0)
    {
        return false;
    }
    //2.获取主机名
    char hostname[256];
    ret = gethostname(hostname, sizeof(hostname));
    if (ret == SOCKET_ERROR)
    {
        WSACleanup();
        return false;
    }
    //3.获取主机ip
    HOSTENT* host = gethostbyname(hostname);
    if (host == NULL)
    {
        WSACleanup();
        return false;
    }

    strcpy(ip, inet_ntoa(*(in_addr*)*host->h_addr_list));
    WSACleanup();
    return true;
}

typedef struct _ASTAT_
{
    ADAPTER_STATUS adapt;
    NAME_BUFFER NameBuff[30];
}ASTAT, *PASTAT;

bool CiticTradeWrapper::GetFirstMac(char * mac)
{
    NCB ncb;
    UCHAR uRetCode;
    ASTAT Adapter;

    memset(&ncb, 0, sizeof(ncb));
    ncb.ncb_command = NCBRESET;
    ncb.ncb_lana_num = 0;

    // 首先对选定的网卡发送一个NCBRESET命令，以便进行初始化
    uRetCode = Netbios(&ncb);
    memset(&ncb, 0, sizeof(ncb));
    ncb.ncb_command = NCBASTAT;
    ncb.ncb_lana_num = 0; // 指定网卡号

    strcpy((char *)ncb.ncb_callname, "* ");
    ncb.ncb_buffer = (unsigned char *)&Adapter;

    // 指定返回的信息存放的变量
    ncb.ncb_length = sizeof(Adapter);

    // 接着，可以发送NCBASTAT命令以获取网卡的信息
    uRetCode = Netbios(&ncb);

    if (uRetCode == 0)
    {
        sprintf(mac, "%02X%02X%02X%02X%02X%02X",
            Adapter.adapt.adapter_address[0],
            Adapter.adapt.adapter_address[1],
            Adapter.adapt.adapter_address[2],
            Adapter.adapt.adapter_address[3],
            Adapter.adapt.adapter_address[4],
            Adapter.adapt.adapter_address[5]);
    }

    return uRetCode == 0;
}

#else
bool CiticTradeWrapper::GetLocalIP(char * ip)
{
    struct ifaddrs * ifAddrStruct = NULL;
    struct ifaddrs * ifa = NULL;
    void * tmpAddrPtr = NULL;

    getifaddrs(&ifAddrStruct);

    for (ifa = ifAddrStruct; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_INET) { // Check it is
                                                   // a valid IPv4 address
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
        else if (ifa->ifa_addr->sa_family == AF_INET6) { // Check it is
                                                         // a valid IPv6 address
            tmpAddrPtr = &((struct sockaddr_in6 *)ifa->ifa_addr)->sin6_addr;
            char addressBuffer[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, tmpAddrPtr, addressBuffer, INET6_ADDRSTRLEN);
            printf("%s IP Address %s\n", ifa->ifa_name, addressBuffer);
        }
    }
    if (ifAddrStruct != NULL)
        freeifaddrs(ifAddrStruct);
    return 0;
}
#endif // WIN32

bool CiticTradeWrapper::connectServer()
{
    char szMsg[512] = {0};
    HSHLPCFGHANDLE hConfig1;
    int iRet = CITICs_HsHlp_LoadConfig(&hConfig1, this->ConfigFile);
    if (iRet)
    {
        printf("加载配置文件失败[Hsconfig.ini] ErrorCdoe=(%d)....\n", iRet);
        return false;
    }
    HSHLPCFGHANDLE handle;
    iRet = CITICs_HsHlp_Init(&handle, hConfig1);
    if (iRet) {
        CITICs_HsHlp_GetErrorMsg(this->HlpHandle, NULL, szMsg);
        printf("初始化连接句柄失败 ErrorCdoe=(%d)....%s\n", iRet, szMsg);
        return false;
    }
    else {
        this->HlpHandle = handle;
    }
    /// 连接服务器
    iRet = CITICs_HsHlp_ConnectServer(this->HlpHandle);
    if (iRet)
    {
        CITICs_HsHlp_GetErrorMsg(this->HlpHandle, NULL, szMsg);
        printf("连接Server失败: ErrorCdoe=(%d) %s....\n", iRet, szMsg);
        return false;
    }
    else
    {
        printf("Connect HSAR OK......\n");
    }
    this->connected = true;
    return true;
}

void ShowErrMsg2(HSHLPHANDLE HlpHandle, int iFunc, Json::Value jso)
{
    int nErr = 0;
    char szMsg[1024] = { 0 };

    CITICs_HsHlp_GetErrorMsg(HlpHandle, &nErr, szMsg);
    jso["code"] = nErr;
    jso["message"] = std::string(szMsg);
    printf("请求业务失败[%d]: Error=(%d) %s\n", iFunc, nErr, szMsg);
}

bool CiticTradeWrapper::Login()
{
    if (!this->connected) {
        this->connectServer();
    }
    printf("\n\n---===客户登录Login===---\n");
    //备注：密码多次送错会冻结账户，十分钟后再试即可。
    int iFunc = 331100;
    CITICs_HsHlp_BeginParam(this->HlpHandle);
    this->SetNecessaryParam(this->HlpHandle);
    CITICs_HsHlp_SetValue(this->HlpHandle, "identity_type", "2");
    CITICs_HsHlp_SetValue(this->HlpHandle, "password_type", "2");
    CITICs_HsHlp_SetValue(this->HlpHandle, "input_content", "1");
    CITICs_HsHlp_SetValue(this->HlpHandle, "op_entrust_way", this->op_entrust_way);
    CITICs_HsHlp_SetValue(this->HlpHandle, "password", this->password);
    CITICs_HsHlp_SetValue(this->HlpHandle, "account_content", this->fund_account);
    CITICs_HsHlp_SetValue(this->HlpHandle, "op_station", this->op_station);
    int iRet = CITICs_HsHlp_BizCallAndCommit(this->HlpHandle, iFunc, NULL);
    if (iRet)
    {
		Json::Value jso;
		jso["code"] = iRet;
		jso["func"] = iFunc;
		jso["message"] = "";
        ShowErrMsg2(this->HlpHandle, iFunc, jso);
        return false;
    }
    //将登陆的返回值保存下来，以后所有的报文都需要
    CITICs_HsHlp_GetValue(HlpHandle, "client_id", this->client_id);
    CITICs_HsHlp_GetValue(HlpHandle, "user_token", this->user_token);
    CITICs_HsHlp_GetValue(HlpHandle, "branch_no", this->branch_no);
    CITICs_HsHlp_GetValue(HlpHandle, "asset_prop", this->asset_prop);
    CITICs_HsHlp_GetValue(HlpHandle, "sysnode_id", this->sysnode_id);
    return true;
}

std::string CiticTradeWrapper::Portfolio()
{
    if (!this->connected) {
        this->connectServer();
    }
    printf("---===客户资金精确查询Portfolio===---\n");
    int iFunc = 332255;
    SetNecessaryParam(this->HlpHandle);

    int iRet = CITICs_HsHlp_BizCallAndCommit(this->HlpHandle, iFunc, NULL);
    Json::Value jso;
    jso["code"] = 0;
    jso["func"] = iFunc;
    jso["message"] = "";

    if (iRet)
    {
        ShowErrMsg2(this->HlpHandle, iFunc, jso);
		return this->_jsonWriter.write(jso);
    }
    int iRow, iCol;
    char szKey[64], szValue[512];
    iRow = CITICs_HsHlp_GetRowCount(HlpHandle);
    iCol = CITICs_HsHlp_GetColCount(HlpHandle);
    if (iRow != 1) {
        printf("获取客户资金精确查询错误\n");
        jso["code"] = 101;
        return this->_jsonWriter.write(jso);
    }

    jso["data"] = Json::Value();
    for(int i=0; i<iCol; i++) {
    	CITICs_HsHlp_GetColName(this->HlpHandle, i, szKey);
        if (strcmp("begin_balance", szKey) == 0) {
        	CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
        	jso["data"]["starting_cash"] = szValue;
        } else if (strcmp("enable_balance", szKey) == 0) {
        	CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
        	jso["data"]["cash"] = szValue;
        } else if (strcmp("market_value", szKey) == 0) {
        	CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
        	jso["data"]["positions_value"] = szValue;
        } else if (strcmp("asset_balance", szKey) == 0) {
        	CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
        	jso["data"]["portfolio_value"] = szValue;
        }
    }
	return this->_jsonWriter.write(jso);

}

std::string CiticTradeWrapper::System()
{
    if (!this->connected) {
        this->connectServer();
    }
    printf("---===系统信息System===---\n");
    int iFunc = 330000;
    SetNecessaryParam(this->HlpHandle);

    int iRet = CITICs_HsHlp_BizCallAndCommit(this->HlpHandle, iFunc, NULL);
    Json::Value jso;
    jso["code"] = 0;
    jso["func"] = iFunc;
    jso["message"] = "";

    if (iRet)
    {
        ShowErrMsg2(this->HlpHandle, iFunc, jso);
		return this->_jsonWriter.write(jso);
    }
    int iRow, iCol;
    char szKey[64], szValue[512];
    iRow = CITICs_HsHlp_GetRowCount(HlpHandle);
    iCol = CITICs_HsHlp_GetColCount(HlpHandle);
    if (iRow != 1) {
        printf("系统信息错误\n");
        jso["code"] = 101;
        return this->_jsonWriter.write(jso);
    }

    jso["data"] = Json::Value();
    for(int i=0; i<iCol; i++) {
    	CITICs_HsHlp_GetColName(this->HlpHandle, i, szKey);
        if (strcmp("init_date", szKey) == 0) {
        	CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
        	jso["data"]["init_date"] = szValue;
        } else if (strcmp("sys_status", szKey) == 0) {
        	CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
        	jso["data"]["sys_status"] = szValue;
        } else if (strcmp("curr_date", szKey) == 0) {
        	CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
        	jso["data"]["curr_date"] = szValue;
        } else if (strcmp("curr_time", szKey) == 0) {
        	CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
        	jso["data"]["curr_time"] = szValue;
        }
    }
	return this->_jsonWriter.write(jso);

}

std::string CiticTradeWrapper::Positions()
{
	if (!this->connected) {
		this->connectServer();
	}
	printf("---===证券持仓查询Positions===---\n");
	int iFunc = 333104;
	SetNecessaryParam(this->HlpHandle);

	int iRet = CITICs_HsHlp_BizCallAndCommit(this->HlpHandle, iFunc, NULL);
	Json::Value jso;
	jso["code"] = 0;
	jso["func"] = iFunc;
	jso["message"] = "";

	if (iRet)
	{
		ShowErrMsg2(this->HlpHandle, iFunc, jso);
		return this->_jsonWriter.write(jso);
	}
	int iRow, iCol;
	char szKey[64], szValue[512];
	iRow = CITICs_HsHlp_GetRowCount(HlpHandle);
	iCol = CITICs_HsHlp_GetColCount(HlpHandle);
	jso["count"] = iRow;
	if (iRow <= 0) {
		return this->_jsonWriter.write(jso);
	}

	jso["data"] = Json::Value();
	for (int j = 0; j < iRow; j++) {
		CITICs_HsHlp_GetNextRow(this->HlpHandle);
		Json::Value pos;
		for (int i = 0; i < iCol; i++) {
			CITICs_HsHlp_GetColName(this->HlpHandle, i, szKey);
			if (strcmp("stock_code", szKey) == 0) {
				CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
				pos["sid"] = szValue;
			}
			else if (strcmp("hold_amount", szKey) == 0) {
				CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
				pos["amount"] = szValue;
			}
			else if (strcmp("enable_amount", szKey) == 0) {
				CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
				pos["available"] = szValue;
			}
			else if (strcmp("cost_price", szKey) == 0) {
				CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
				pos["cost_basis"] = szValue;
			}
			else if (strcmp("last_price", szKey) == 0) {
				CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
				pos["last_sale_price"] = szValue;
			}
		}
		jso["data"].append(pos);
	}
	return this->_jsonWriter.write(jso);
}

std::string CiticTradeWrapper::Order(const char * stock_code, float price, int volume)
{
	if (!this->connected) {
		this->connectServer();
	}
	printf("---===普通下单Order===---\n");
	int iFunc = 333002;
	SetNecessaryParam(this->HlpHandle);
    if (stock_code[0] == '6') {
        CITICs_HsHlp_SetValue(this->HlpHandle, "exchange_type", "1");//1表示上海A股
    }
    else {
        CITICs_HsHlp_SetValue(this->HlpHandle, "exchange_type", "2");//2表示深圳A股
    }
	CITICs_HsHlp_SetValue(this->HlpHandle, "stock_code", stock_code);//股票代码
    if (volume > 0) {
        CITICs_HsHlp_SetValue(this->HlpHandle, "entrust_bs", "1");//1买 2卖
        CITICs_HsHlp_SetValue(this->HlpHandle, "entrust_amount", std::to_string(volume).c_str());//数量
    }
    else {
        CITICs_HsHlp_SetValue(this->HlpHandle, "entrust_bs", "2");//1买 2卖
        CITICs_HsHlp_SetValue(this->HlpHandle, "entrust_amount", std::to_string(-volume).c_str());//数量
    }
	CITICs_HsHlp_SetValue(this->HlpHandle, "entrust_price", std::to_string(price).c_str());//价格
	
	CITICs_HsHlp_SetValue(this->HlpHandle, "entrust_prop", "0");// 普通买卖
	CITICs_HsHlp_SetValue(this->HlpHandle, "batch_no", "0");//0表示单笔订单
	CITICs_HsHlp_SetValue(this->HlpHandle, "entrust_type", "0");
	int iRet = CITICs_HsHlp_BizCallAndCommit(this->HlpHandle, iFunc, NULL);
	Json::Value jso;
	jso["code"] = 0;
	jso["func"] = iFunc;
	jso["message"] = "";
    
	if (iRet)
	{
		ShowErrMsg2(this->HlpHandle, iFunc, jso);
		return this->_jsonWriter.write(jso);
	}
    jso["data"] = Json::Value();
    char entrust_no[32] = { 0 };
    CITICs_HsHlp_GetValue(HlpHandle, "entrust_no", entrust_no);
    jso["data"]["order_id"] = entrust_no;
	return this->_jsonWriter.write(jso);
}

std::string CiticTradeWrapper::Transactions()
{
    if (!this->connected) {
        this->connectServer();
    }
    printf("---===证券成交查询Transactions===---\n");
    int iFunc = 333102;
    SetNecessaryParam(this->HlpHandle);

    int iRet = CITICs_HsHlp_BizCallAndCommit(this->HlpHandle, iFunc, NULL);
    Json::Value jso;
    jso["code"] = 0;
    jso["func"] = iFunc;
    jso["message"] = "";

    if (iRet)
    {
        ShowErrMsg2(this->HlpHandle, iFunc, jso);
        return this->_jsonWriter.write(jso);
    }
    int iRow, iCol;
    char szKey[64], szValue[512];
    iRow = CITICs_HsHlp_GetRowCount(HlpHandle);
    iCol = CITICs_HsHlp_GetColCount(HlpHandle);
    jso["count"] = iRow;
    if (iRow <= 0) {
        return this->_jsonWriter.write(jso);
    }

    jso["data"] = Json::Value();
    for (int j = 0; j < iRow; j++) {
        CITICs_HsHlp_GetNextRow(this->HlpHandle);
        Json::Value order;
        std::string date;
        std::string time;
        for (int i = 0; i < iCol; i++) {
            CITICs_HsHlp_GetColName(this->HlpHandle, i, szKey);
            if (strcmp("stock_code", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["sid"] = szValue;
            }
            else if (strcmp("entrust_status", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["status"] = szValue;
            }
            else if (strcmp("entrust_amount", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["amount"] = szValue;
            }
            else if (strcmp("business_amount", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["filled"] = szValue;
            }
            else if (strcmp("entrust_no", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["order_id"] = szValue;
            }
            else if (strcmp("entrust_price", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["price"] = szValue;
            }
            else if (strcmp("entrust_time", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                int len = strlen(szValue);
                if (len < 6) {  //92020
                    time = std::string("0") + szValue;
                }
                else {
                    time = szValue;
                }
            }
            else if (strcmp("entrust_date", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                date = szValue;
            }
        }
        order["dt"] = date + " " + time;
        jso["data"].append(order);
    }
    return this->_jsonWriter.write(jso);
}

std::string CiticTradeWrapper::Orders()
{
    if (!this->connected) {
        this->connectServer();
    }
    printf("---===证券委托查询Orders===---\n");
    int iFunc = 333101;
    SetNecessaryParam(this->HlpHandle);

    int iRet = CITICs_HsHlp_BizCallAndCommit(this->HlpHandle, iFunc, NULL);
    Json::Value jso;
    jso["code"] = 0;
    jso["func"] = iFunc;
    jso["message"] = "";

    if (iRet)
    {
        ShowErrMsg2(this->HlpHandle, iFunc, jso);
        return this->_jsonWriter.write(jso);
    }
    int iRow, iCol;
    char szKey[64], szValue[512];
    iRow = CITICs_HsHlp_GetRowCount(HlpHandle);
    iCol = CITICs_HsHlp_GetColCount(HlpHandle);
    jso["count"] = iRow;
    if (iRow <= 0) {
        return this->_jsonWriter.write(jso);
    }

    jso["data"] = Json::Value();
    for (int j = 0; j < iRow; j++) {
        CITICs_HsHlp_GetNextRow(this->HlpHandle);
        Json::Value order;
        std::string date;
        std::string time;
        for (int i = 0; i < iCol; i++) {
            CITICs_HsHlp_GetColName(this->HlpHandle, i, szKey);
            if (strcmp("stock_code", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["sid"] = szValue;
            }
            else if (strcmp("entrust_status", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["status"] = szValue;
            }
            else if (strcmp("entrust_amount", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["amount"] = szValue;
            }
            else if (strcmp("business_amount", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["filled"] = szValue;
            }
            else if (strcmp("entrust_no", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["order_id"] = szValue;
            }
            else if (strcmp("entrust_price", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                order["price"] = szValue;
            }
            else if (strcmp("entrust_time", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                int len = strlen(szValue);
                if (len < 6) {  //92020
                    time = std::string("0") + szValue;
                }
                else {
                    time = szValue;
                }
            }
            else if (strcmp("entrust_date", szKey) == 0) {
                CITICs_HsHlp_GetValueByIndex(this->HlpHandle, i, szValue);
                date = szValue;
            }
        }
        order["dt"] = date + " " + time;
        jso["data"].append(order);
    }
    return this->_jsonWriter.write(jso);
}

std::string CiticTradeWrapper::CancelOrder(const char * orderId)
{
    if (!this->connected) {
        this->connectServer();
    }
    printf("---===委托撤单CancelOrder===---\n");
    int iFunc = 333017;
    SetNecessaryParam(this->HlpHandle);
    CITICs_HsHlp_SetValue(this->HlpHandle, "entrust_no", orderId);
    CITICs_HsHlp_SetValue(this->HlpHandle, "batch_flag", "0");// 0：单笔撤单
    CITICs_HsHlp_SetValue(this->HlpHandle, "locate_entrust_no", orderId);
    int iRet = CITICs_HsHlp_BizCallAndCommit(this->HlpHandle, iFunc, NULL);
    Json::Value jso;
    jso["code"] = 0;
    jso["func"] = iFunc;
    jso["message"] = "";
    /**
    *委托状态。'0'	未报
    '1'	待报
    '2'	已报
    '3'	已报待撤
    '4'	部成待撤
    '5'	部撤
    '6'	已撤
    '7'	部成
    '8'	已成
    '9'	废单
    */
    //只有entrust_status为0、2或7时才能撤单，不然会报“委托状态错误”
    if (iRet)
    {
        ShowErrMsg2(this->HlpHandle, iFunc, jso);
        return this->_jsonWriter.write(jso);
    }
    jso["data"] = Json::Value();
    char entrust_no[32] = { 0 };
    CITICs_HsHlp_GetValue(HlpHandle, "entrust_no", entrust_no);
    jso["data"]["order_id"] = entrust_no;
    return this->_jsonWriter.write(jso);
}

CiticTradeWrapper::CiticTradeWrapper(const char* account, const char* password, const char * configFile, const char* op_entrust_way, const char* clientName) {
    strncpy(this->fund_account, account, strlen(account));
    strncpy(this->password, password, strlen(password));
    strncpy(this->op_entrust_way, op_entrust_way, strlen(op_entrust_way));
    strncpy(this->ClientName, clientName, strlen(clientName));
    strncpy(this->ConfigFile, configFile, strlen(configFile));
    assert(this->GetLocalIP(this->IP) && "获取本机IPC失败，请手动填入内网IP至对应字符串。\n");
    assert(this->GetFirstMac(this->MAC) && "获取本机MAC失败，请手动填入本机MAC至对应字符串。\n");
    strcat(this->op_station, "TYJR-");//默认前缀，请不要修改
    strcat(this->op_station, this->ClientName);
    strcat(this->op_station, " IP.");
    strcat(this->op_station, this->IP);
    strcat(this->op_station, " MAC.");
    strcat(this->op_station, MAC);
}


CiticTradeWrapper::~CiticTradeWrapper()
{
    if (this->HlpHandle != NULL) {
        CITICs_HsHlp_DisConnect(this->HlpHandle);
        CITICs_HsHlp_Exit(this->HlpHandle);
    }
}
