#include <iostream>
//#include "ICEClient.h"
#include <stdio.h>
#include <string.h>
//#include <dlfcn.h>
#include "ISMasterApi.h"
#include "IntelAssitant.pb.h"

#if WIN32
#include <Windows.h>
#endif

//#include "loggerWrapper.h"
//2016-07-13am ~ 2016-07-13pm  

using namespace std;

void ConvertGBKToUtf8(std::string& amp, std::string strGBK);
void ConvertUtf8ToGBK(std::string&amp, std::string strUtf8);
char *getFileContents(const char *fileName,int *fileLength);
bool MakeInParasProto(std::string sIn,std::string &sOut);

int main(int argc,char **argv)
{
	//Part of input-verify 
	if( argc < 2)
	{
		printf("Usage: %s QueryText\r\n",argv[0]);
		return -1;
	}
	
	//----------------------------------------------------------------
	//Init Function Testing
	//LogicControl logicMan;
	int nFileLength = 0;
	char *pConfigBuffer = getFileContents("/data/ythu4/Source/Cfg/IntelligentAssistant.cfg",&nFileLength);
	if( NULL == pConfigBuffer )
	{
		printf("获取配置文件出错!\r\n");
		return -1;
	}
	pConfigBuffer[nFileLength-1] = '\0';
	std::string sConfig(pConfigBuffer);
	delete []pConfigBuffer;
	if ( 0 != Init(sConfig.c_str()))
	{
		cout << "初始化失败!" <<endl;
		return -1;
	}
	
	//------------------------------------------------------------------
	// PreProcess Function Testing
	
	std::string sPreIn = "开通全球通58元套餐"; //此处随意输入一段字符作为测试使用
	std::string sPreOut = "";
	int type = (0x0001 | 0x0002);      //------------------?

	if ( 0 != PreProcess(sPreIn.c_str(),(char *)sPreOut.c_str(),type))  //通过preProcessP这个指针调用函数 PreProcess(const char*)
	{
		cout << "预处理失败!" <<endl;
		return -1;
	}
	
	
	//------------------------------------------------------------------
	//Query Funciton testing
	std::string sIn;
	std::string sQueryText(argv[1]);
	//INFO(sQueryText);
	if( !MakeInParasProto(sQueryText,sIn))
	{
		cout<<"fail to make InParasProto !"<<endl;
		return -1;
	}
	std::string sOut;
	char szOut[10240] = {0};
	int nOut = 0;
	if ( 0 != Query(sIn.c_str(),szOut,&nOut))
	{
		return -1;
	}
	ISMaster::OutParas oResult;
	sOut.append(szOut,nOut);
	oResult.ParseFromString(sOut);
	//INFO(oResult.Utf8DebugString());
	cout << "--------------------------------------------------" <<endl;
	cout << oResult.Utf8DebugString() << endl;
	cout << "--------------------------------------------------" <<endl;

	
	//---------------------------------------------------------------------------
	//Uninit Function Testing
	if(0!=Uninit())
	{
		cout<<"fail to Uninit !"<<endl;
		return -1;
	}
	
	
	return 0;
}



void ConvertGBKToUtf8( std::string& amp, std::string strGBK )
{
#ifdef WIN32
	int len=MultiByteToWideChar(CP_ACP, 0, (LPCSTR)strGBK.c_str(), -1, NULL,0); 
	unsigned short * wszUtf8 = new unsigned short[len+1]; 
	memset(wszUtf8, 0, len * 2 + 2); 
	MultiByteToWideChar(CP_ACP, 0, (LPCSTR)strGBK.c_str(), -1, (LPWSTR)wszUtf8, len); 
	len = WideCharToMultiByte(CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, NULL, 0, NULL, NULL); 
	char *szUtf8=new char[len + 1]; 
	memset(szUtf8, 0, len + 1); 
	WideCharToMultiByte (CP_UTF8, 0, (LPCWSTR)wszUtf8, -1, szUtf8, len, NULL,NULL); 
	//strGBK = szUtf8; 
	amp=szUtf8;
	delete[] szUtf8; 
	delete[] wszUtf8; 
#endif
}

void ConvertUtf8ToGBK( std::string&amp, std::string strUtf8 )
{
#ifdef WIN32
	int len=MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)strUtf8.c_str(), -1, NULL,0); 
	unsigned short * wszGBK = new unsigned short[len+1]; 
	memset(wszGBK, 0, len * 2 + 2); 
	MultiByteToWideChar(CP_UTF8, 0, (LPCSTR)strUtf8.c_str(), -1, (LPWSTR)wszGBK, len); 
	len = WideCharToMultiByte(CP_ACP, 0, (LPCWSTR)wszGBK, -1, NULL, 0, NULL, NULL); 
	char *szGBK=new char[len + 1]; 
	memset(szGBK, 0, len + 1); 
	WideCharToMultiByte (CP_ACP, 0, (LPCWSTR)wszGBK, -1, szGBK, len, NULL,NULL); 
	//strUtf8 = szGBK; 
	amp=szGBK;
	delete[] szGBK; 
	delete[] wszGBK; 
#endif
}
char *getFileContents(const char *fileName,int *fileLength)
{
	FILE *fp = NULL;
	if((fp = fopen(fileName,"rb")) == NULL)
	{
		return NULL;
	}
	fseek(fp,0,SEEK_SET);
	fseek(fp,0,SEEK_END);
	int fileSize = ftell(fp);
	*fileLength = fileSize;
	char *pchBuffer = new char[fileSize];
	fseek(fp,0,SEEK_SET);
	memset(pchBuffer,0,fileSize);
	fread(pchBuffer,fileSize,1,fp);
	fclose(fp);
	return pchBuffer;
}

bool MakeInParasProto(std::string sIn,std::string &sOut)
{
	ISMaster::InParas paras;
	paras.set_tid("10000001");
	paras.set_userid("200001231");
	// 1 NETWORKSTANDRD::NET_4G
	// 2 CHANNELTYPE::SHORTMESSAGE
	paras.set_networkstandard(1);
	paras.set_channeltype(2);
	paras.set_text(sIn);
	//paras.set_preprocesstype(7);
	paras.set_actiontype("normal");
	cout << paras.Utf8DebugString() <<endl;
	return paras.SerializeToString(&sOut);
}