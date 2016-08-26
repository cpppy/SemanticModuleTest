#include <iostream>
#include <vector>
#include <fstream>
#include <string>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

//#include "ICEClient.h"
#include "IACommonDefines.h"
#include "ISMasterApi.h"
#include "IntelAssitant.pb.h"

#if WIN32
#include <Windows.h>
#endif

#include "ThreadPool.h"
#include "loggerWrapper.h"
#include "uuid_t.h"
#include "json/json.h"

using namespace std;

void ConvertGBKToUtf8(std::string& amp, std::string strGBK);
void ConvertUtf8ToGBK(std::string&amp, std::string strUtf8);
char *getFileContents(const char *fileName,int *fileLength);
bool MakeInParasProto(std::string sQuery,std::string userId,int actionType, std::string &sOut);
int WorkingFunc(string & sConfig,Json::Value &tmpJson,int loopNum);
int Test_WorkingFunc(std::string sQuery,int actionType,int loopNum);
//bool DumpToFile(std::string &sContent,int nLength,std::string sFileName);
string readDefaultInfo(Json::Value &root,ISMaster::InParas& in);
string writeIntoProto(Json::Value &root,ISMaster::InParas& in,vector<std::string> strs);   // add*******

void SetActionType(ISMaster::InParas& in,int type);
string getDelSession(Json::Value& root);
string getOrderLiuLiang(Json::Value& root);
string getOrderLiuLiang2(Json::Value& root,vector<std::string> strs);       // add*******
string getDaoHang(Json::Value& root);
string getReply_oprional(Json::Value& root);
string getReply_Y(Json::Value& root);

void split(string str,vector<string>& res);                // add*******
string trim(const string& str);                                  // add*******
string deln(string str);                                            // add*******

vector<std::string> orgStr;
vector<vector<std::string> > qIn;  //定义全局变量
vector<std::string> qOut;
Json::Value tmpJson;
//struct timeval tv;
pthread_mutex_t mut;

void* test(void* args) 
{
	int pi=*((int*)args);
	string protobuf; 
	timeval starttime,endtime;
	vector<string> strs;
	for(int i=0;i<qIn.size();++i) 
	{
		cout<<"-----------------------------------********************   "<<i<<"   ***********************"<<endl;
		cout<<qIn.size()<<"*"<<qIn[0].size()<<endl;
		
		strs=qIn[i];
		strs[1]=to_string(atoi(strs[1].c_str()) + pi);
		cout<<"phone num   "<<strs[1]<<endl;
		protobuf=getOrderLiuLiang2(tmpJson,strs);
		cout<<(std::string)qIn[i][2]<<endl;
			
		char szOut[4048] = {0}; 
		int nOut=0;
		string sOut;
		ISMaster::InParas in;
		in.ParseFromString(protobuf);
		std::cout<<"****本次query输入信息******\n"<<in.Utf8DebugString()<<std::endl;
		gettimeofday(&starttime,0);                       //计时开始-----------------------------------------------------
		if ( 0 != Query(protobuf.c_str(),szOut,&nOut))
		{
			cout << "Query失败!" <<endl;
		}
		gettimeofday(&endtime,0);   // 计时结束-------------------------------------------------------
		ISMaster::SHNote oResult;
		sOut.clear();
		sOut.append(szOut,nOut);
		oResult.ParseFromString(sOut);
		DEBUG("本次query得到的结果:\n"<<oResult.Utf8DebugString());
		
		std::string sop;
		sop = oResult.Utf8DebugString();
		//sop=deln(sop);
		//orgStr[i]=deln(orgStr[i]);
		   
		double td = 1000000*(endtime.tv_sec - starttime.tv_sec) + endtime.tv_usec - starttime.tv_usec;

		string timestr="";
		stringstream ss;
		ss<<td;
		ss>>timestr;
		ss.clear();
		
		
		pthread_mutex_lock(&mut);
		qOut.push_back("testID: " + to_string(i)+"\t"+qIn[i][2]+"\t"+ "Time_Used: "+timestr);
		//qOut.push_back("testID: " + to_string(i)+"\n"+orgStr[i]+"\n"+sop+"\n"+"Time_Used : "+timestr+"\n\n");
		if(qOut.size()>200)
		{
			ofstream queryOutfile("queryOutput.txt",ios::app);
			if(!queryOutfile)
			{
				cerr<<"fail to open file 'queryOutput.txt'"<<endl;
				return -1;
			}
			for(int i=0;i<qOut.size();++i)
			{
				queryOutfile<<qOut[i]<<endl;
			}
			queryOutfile.close();
			qOut.clear();
		}
		pthread_mutex_unlock(&mut);
			
	}
	
	//pthread_mutex_lock(&mut);
	//cout<<"this is a thread "<<*((int*)args)<<endl;
	//pthread_mutex_unlock(&mut);
	//int status = 10 + *((int*)args);
	pthread_exit(0);
}

class cInparas
{
public:
	string tid;
	string userid;
	string text;
	string priority;
	string networkstandard;
	string channeltype;
	string actiontype;
	 
	void vec2class(vector<std::string> vec)
	{
		tid = vec[0];
		userid = vec[1];
		text = vec[2];
		priority = vec[3];
		networkstandard = vec[4];
		channeltype = vec[5];
		actiontype = vec[6];
	}
	
	void printClass()
	{
		cout<<"tid : "<<tid<<endl;
		cout<<"userid : "<<userid<<endl;
		cout<<"text : "<<text<<endl;
		cout<<"priority : "<<priority<<endl;
		cout<<"networkstandard : "<<networkstandard<<endl;
		cout<<"channeltype : "<<channeltype<<endl;
		cout<<"actiontype : "<<actiontype<<endl;
	}
	
};



int main(int argc,char **argv)
{
	

	
	if( argc <2)
	{
//		printf("Usage: %s QueryText actionType[1|2|3|4] threadNum-线程数 loopNum-循环次数\r\n",argv[0]);
		printf("Usage: %s optional \r\njson中操作类型(delsession,orderliuliang,daohang,reply_optional,reply_y)\n \
		注：如果输入 thread 2 3 表示使用多线程测试性能,使用2个线程循环3次\n",argv[0]);
		return -1;
	}

	cout<<"passline---------------------------------------------------------"<<endl;
	// read queryInput.txt 
	//vector<std::string> orgStr;
	//vector<vector<std::string> > qIn;//restore the inparas structrue data
	//vector<std::string> qOut;
	vector<std::string> tmp; //used to split one line to several parameters temperary
	//vector<std::string> outstrs;
	ifstream queryInfile("resinp_function1.1.txt");
	std::string qline="";
	if(!queryInfile.is_open())
	{
		cout<<"Error opening 'Input_txt'"<<endl;
		exit(-1);
	}
	else{
		cout<<"-----------------------------reading Input_txt "<<endl;
	}
	
	while(getline(queryInfile,qline))
	{
		tmp.clear();
		qline=deln(qline);
		//cout<<qline<<endl;
		orgStr.push_back(qline);
		qline = trim(qline);
		//cout<<qline<<endl;
		split(qline,tmp);
		qIn.push_back(tmp);
		
	}
	
	cout<<qIn.size()<<'*'<<qIn[0].size()<<"--------------------  -----------size of 2vec qIn "<<endl;
	
	queryInfile.close();  
	cout<< "--------------------------reading queryInput.txt finished! ---------------------------"<<endl; 
	

	
	string operator_type=argv[1];
	int nFileLength = 0;
	char *pConfigBuffer = getFileContents("../Cfg/IntelligentAssistant.cfg",&nFileLength);
	if( NULL == pConfigBuffer )
	{
		printf("获取配置文件出错!\r\n");
		return -1;
	}
	pConfigBuffer[nFileLength-1] = '\0';
	std::string sConfig(pConfigBuffer);
	delete []pConfigBuffer;

	if ( operator_type == "test" )
	{
		if( argc != 4)
		{
			printf("Usage %s test queryTest actionType\r\n",argv[0]);
			return -1;
		}
		if ( 0 != Init(sConfig.c_str()))
		{
			cout << "初始化失败!" <<endl;
			return -1;
		}
		std::string sQuery = argv[2];
		int actionType = atoi(argv[3]);
		return Test_WorkingFunc(sQuery,actionType,0);
	}

	int nLength = 0;
	char *pYuWuConfigBuffer = getFileContents("../Cfg/YeWuConfig.json",&nLength);
	if( NULL == pYuWuConfigBuffer )
	{
		printf("获取业务配置文件出错!\r\n");
		return -1;
	}
	//pYuWuConfigBuffer[nLength-1] = '\0';
	std::string YeWuConfig(pYuWuConfigBuffer);
	delete []pYuWuConfigBuffer;
	
//	std::cout<<YeWuConfig<<endl;
	Json::Reader reader;
	Json::Value root;
	string protobuf; 
	
	
	

	/*
	if(reader.parse(YeWuConfig.c_str(),root))
	{
		if(!root["cfg"].isNull())
		{
			Json::Value tmpJson=root["cfg"];

			if(operator_type=="delsession")
				protobuf=getDelSession(tmpJson);
			else if(operator_type=="orderliuliang")
				protobuf=getOrderLiuLiang(tmpJson);
			else if(operator_type=="daohang")
				protobuf=getDaoHang(tmpJson);
			else if(operator_type=="reply_optional")
				protobuf=getReply_oprional(tmpJson);
			else if(operator_type=="reply_y")
				protobuf=getReply_Y(tmpJson);
			
		}
	}
	else
	{
		printf("解析业务配置文件出错!\r\n");
		return -1;
	}
	*/
	
	if(argc==2)
	{
		//Json::Value tmpJson; //定义到全局
		if(reader.parse(YeWuConfig.c_str(),root))
		{
			if(!root["cfg"].isNull())
			{
				tmpJson=root["cfg"];
				
			}
		}
		else
		{
			printf("解析业务配置文件出错!\r\n");
			return -1;
		}
		
		
		if ( 0 != Init(sConfig.c_str()))
		{
			cout << "初始化失败!" <<endl;
			return -1;
		}
		
		
		int n=50;  //线程数
		pthread_t tids[n];
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_JOINABLE);
		
		pthread_mutex_init(&mut,NULL);
		
		//vector<int> iarr(n,0);
		int iarr[n];
		//vector<int>::iterator iter = iarr.begin();
		for(int i=0;i<n;++i) {
			iarr[i] = i;
			int ret=pthread_create(&tids[i],NULL,test,(void*)&(iarr[i]));
			//cout<<"tid = "<<tids[i]<<endl;
			if(ret!=0) {
				cout<<" this thread id get wrong "<<endl;
			}
		}
		
		pthread_attr_destroy(&attr);
		
		void* status;
		for(int i=0;i<n;++i) {
			//pthread_join(tids[i],NULL);
			int ret = pthread_join(tids[i],&status);
			if(ret!= 0){
				cout<<"jion fail "<<ret<<endl;
			}
			else cout<<"jion success"<<(long)status<<endl;
		}
		
		pthread_mutex_destroy(&mut);
		
		/*
		for(int i=0;i<qIn.size();++i) 
		{
			cout<<"-----------------------------------********************   "<<i<<"   ***********************"<<endl;
			gettimeofday(&tv,NULL);                       //计时开始-----------------------------------------------------
			unsigned long start_time=tv.tv_usec; 
			
			protobuf=getOrderLiuLiang2(tmpJson,qIn[i]);
			cout<<(std::string)qIn[i][2]<<endl;
				
			char szOut[4048] = {0}; 
			int nOut=0;
			string sOut;
			ISMaster::InParas in;
			in.ParseFromString(protobuf);
			std::cout<<"****本次query输入信息******\n"<<in.Utf8DebugString()<<std::endl;
			if ( 0 != Query(protobuf.c_str(),szOut,&nOut))
			{
				cout << "Query失败!" <<endl;
				return -1;
			}
			ISMaster::SHNote oResult;
			sOut.clear();
			sOut.append(szOut,nOut);
			oResult.ParseFromString(sOut);
			DEBUG("本次query得到的结果:\n"<<oResult.Utf8DebugString());
			
			std::string sop;
			sop = oResult.Utf8DebugString();
			//sop=deln(sop);
			orgStr[i]=deln(orgStr[i]);
			
			gettimeofday(&tv,NULL);   // 计时结束-------------------------------------------------------
			unsigned long end_time=tv.tv_usec;         
			unsigned long td = end_time-start_time;

			cout<<"start_time: "<<start_time<<endl;
			cout<<"end_time："<<end_time<<endl;

			string timestr="";
			stringstream ss;
			ss<<td;
			ss>>timestr;
			ss.clear();
			
			//qOut.push_back("testID: " + to_string(i)+"\n"+orgStr[i]+"\n"+sop+"\n"+"Time_Used : "+timestr+"\n\n");
			qOut.push_back("testID: " + to_string(i)+"         "+qIn[i][2]+"        Time_Used: "+timestr+'\n');

		}
		*/
		
		ofstream queryOutfile("queryOutput.txt",ios::app);
		if(!queryOutfile)
		{
			cerr<<"fail to open file 'queryOutput.txt'"<<endl;
			return -1;
		}
		for(int i=0;i<qOut.size();++i)
		{
			queryOutfile<<qOut[i]<<endl;
		}
		queryOutfile.close();
		
		
		Uninit();
		return 0;
		
	}
	
	
	else if(argc==4 && operator_type=="thread")
	{
		int threadNum = atoi(argv[2]);
		int loopNum = atoi(argv[3]);
		if(threadNum<1 || loopNum<1)
		{
			std::cout<<"输入参数小于1，错误"<<endl;
		}		
		
		BaseLib::ThreadPool pool;
		pool.start(threadNum);
		threadNum=1;
		for ( int i=0;i<threadNum;i++)
		{
			pool.run(boost::bind(&WorkingFunc,sConfig,root["cfg"],loopNum));
		}
	}
	else
	{
		std::cout<<"输入参数不对"<<endl;
		return -1;
	}
	
	exit(0);
// 	std::string sUTF8(sOut);
// #ifdef WIN32
// 	ConvertGBKToUtf8(sUTF8,sOut);
// #endif
// 	INFO(sUTF8);
#if 0
	::setlocale(LC_ALL, ".ACP");
	//设置属性
	ICEClient iceClient;
	if( 0 != iceClient.NLICInit("./isc.cfg"))
	{
		return -1;
	}
	//const char * sentence = argv[1];
	const char * sentence = "查询话费";
	//<帮我查询gprs详单>,<nbest=3;org=001;>,<35>
	const char * par = "userid=U01;encode=utf-8;rse=utf-8;nbest=3;org=001;";
	const char * user_id = "";

	std::string sQuery("");
	ConvertGBKToUtf8(sQuery,std::string(sentence));
	// 语义解析
	int ret = 0;
	std::string sNLI_result = iceClient.NLICInterpret(sQuery.c_str(), par, user_id, ret);
	if (sNLI_result.empty())
	{
		printf("结果错误!\r\n");
		return -1;
	}
	std::string sResult("");
	ConvertUtf8ToGBK(sResult,sNLI_result);
	cout << "--------------------------------------------------" <<endl;
	printf("%s\r\n",sResult.c_str());
	cout << "--------------------------------------------------" <<endl;
#endif


	return 0;
}
void SetActionType(ISMaster::InParas& in,int type)
{
	switch(type)
	{
	case 1:
		in.set_actiontype(ISMaster::ACTIONTYPE_NORMAL);
		break;
	case 2:
		in.set_actiontype(ISMaster::ACTIONTYPE_CLEAR);
		break;
	case 3:
		in.set_actiontype(ISMaster::ACTIONTYPE_REPLACE);
		break;
	case 4:
		in.set_actiontype(ISMaster::ACTIONTYPE_ADD);
		break;
	default:
		break;
	}
}

string readDefaultInfo(Json::Value &root,ISMaster::InParas& in)
{
	if(!root["tid"].isNull())
		in.set_tid(root["tid"].asString());

	if(!root["text"].isNull())
		in.set_text(root["text"].asString());

	if(!root["userinfo"].isNull())
	{
		ISMaster::Userdata *user=in.mutable_userinfo();
		if(!root["userinfo"]["userid"].isNull())
			user->set_userid(root["userinfo"]["userid"].asString());
		if(!root["userinfo"]["city"].isNull())
			user->set_city(root["userinfo"]["city"].asString());
		if(!root["userinfo"]["brand"].isNull())
			user->set_brand(root["userinfo"]["brand"].asString());
		if(!root["userinfo"]["networkstandard"].isNull())
			user->set_networkstandard(root["userinfo"]["networkstandard"].asString());
		if(!root["userinfo"]["channeltype"].isNull())
			user->set_channeltype(root["userinfo"]["channeltype"].asString());					
	}

	if(!root["actiontype"].isNull())
		SetActionType(in,root["actiontype"].asInt());

	if(!root["exbusiness"].isNull())
	{

	}

	if(!root["preprocesstype"].isNull())
		in.set_preprocesstype(root["preprocesstype"].asInt());
	
	if(!root["priority"].isNull())
		in.set_priority(root["priority"].asString());

	if(!root["sessionid"].isNull())
		in.set_sessionid(root["sessionid"].asString());

	if(!root["answer"].isNull())
	{
		ISMaster::Answer *answer=in.mutable_answer();
		if(!root["answer"]["type"].isNull())
			answer->set_type(root["answer"]["type"].asString());
		if(!root["answer"]["content"].isNull())
			answer->set_content(root["answer"]["content"].asString());
		if(!root["answer"]["iterms"].isNull())
		{
			for(int i=0;i<root["answer"]["iterms"].size();i++)
			{
				ISMaster::Pair *pair=answer->add_iterms();
				if(!root["answer"]["iterms"][i]["key"].isNull())
					pair->set_key(root["answer"]["iterms"][i]["key"].asString());
				if(!root["answer"]["iterms"][i]["value"].isNull())
					pair->set_value(root["answer"]["iterms"][i]["value"].asString());
			}	
		}
	}

	if(!root["formatext"].isNull())
		in.set_formatext(root["formatext"].asString());

	if(!root["orignaltext"].isNull())
		in.set_orignaltext(root["orignaltext"].asString());

	string str=in.SerializeAsString();
//	std::cout<<"****protobuf info******\n"<<in.Utf8DebugString()<<std::endl;;
	return str;

}


string writeIntoProto(Json::Value &root,ISMaster::InParas& in,vector<std::string> strs)
{
	
	cInparas cIn;
	cIn.vec2class(strs);
	cIn.printClass();                          //--------------------------print input info---------
	cout<<" cIn.text :   "<<cIn.text<<endl;
	
	if(!root["tid"].isNull())
		in.set_tid(root["tid"].asString());

	if(!root["text"].isNull())
	{
		//in.set_text(root["text"].asString());
		//in.set_text(cIn.text);
		in.set_text(strs[2]);              //----------------------set  message into inparas
	}
		

	if(!root["userinfo"].isNull())
	{
		ISMaster::Userdata *user=in.mutable_userinfo();
		if(!root["userinfo"]["userid"].isNull())
			user->set_userid(strs[1]);
		if(!root["userinfo"]["city"].isNull())
			user->set_city(root["userinfo"]["city"].asString());
		if(!root["userinfo"]["brand"].isNull())
			user->set_brand(root["userinfo"]["brand"].asString());
		if(!root["userinfo"]["networkstandard"].isNull())
			user->set_networkstandard(root["userinfo"]["networkstandard"].asString());
		if(!root["userinfo"]["channeltype"].isNull())
			user->set_channeltype(root["userinfo"]["channeltype"].asString());					
	}

	if(!root["actiontype"].isNull())
		SetActionType(in,root["actiontype"].asInt());

	if(!root["exbusiness"].isNull())
	{

	}

	if(!root["preprocesstype"].isNull())
		in.set_preprocesstype(root["preprocesstype"].asInt());
	
	if(!root["priority"].isNull())
		in.set_priority(root["priority"].asString());

	if(!root["sessionid"].isNull())
		in.set_sessionid(root["sessionid"].asString());

	if(!root["answer"].isNull())
	{
		ISMaster::Answer *answer=in.mutable_answer();
		if(!root["answer"]["type"].isNull())
			answer->set_type(root["answer"]["type"].asString());
		if(!root["answer"]["content"].isNull())
			answer->set_content(root["answer"]["content"].asString());
		if(!root["answer"]["iterms"].isNull())
		{
			for(int i=0;i<root["answer"]["iterms"].size();i++)
			{
				ISMaster::Pair *pair=answer->add_iterms();
				if(!root["answer"]["iterms"][i]["key"].isNull())
					pair->set_key(root["answer"]["iterms"][i]["key"].asString());
				if(!root["answer"]["iterms"][i]["value"].isNull())
					pair->set_value(root["answer"]["iterms"][i]["value"].asString());
			}	
		}
	}

	if(!root["formatext"].isNull())
		in.set_formatext(root["formatext"].asString());

	if(!root["orignaltext"].isNull())
		in.set_orignaltext(root["orignaltext"].asString());

	string str=in.SerializeAsString();
//	std::cout<<"****protobuf info******\n"<<in.Utf8DebugString()<<std::endl;;
	return str;

}

string getDelSession(Json::Value& root)
{
	ISMaster::InParas in;
	string retStr;
	if(!root["DelSession"].isNull())
	{
		Json::Value tmpJson=root["DelSession"];
		retStr=readDefaultInfo(tmpJson,in);
	}
	return retStr;
}


string getOrderLiuLiang(Json::Value& root)
{
	ISMaster::InParas in;
	string retStr;
	if(!root["orderLiuLiang"].isNull())
	{
		Json::Value tmpJson=root["orderLiuLiang"];
		retStr=readDefaultInfo(tmpJson,in);
	}
	return retStr;
}

string getOrderLiuLiang2(Json::Value& root,vector<std::string> strs)
{
	ISMaster::InParas in;
	string retStr;
	if(!root["orderLiuLiang"].isNull())
	{
		Json::Value tmpJson=root["orderLiuLiang"];
		retStr=writeIntoProto(tmpJson,in,strs);
	}
	return retStr;
}



string getDaoHang(Json::Value& root)
{
	ISMaster::InParas in;
	string retStr;
	if(!root["DaoHang"].isNull())
	{
		Json::Value tmpJson=root["DaoHang"];
		retStr=readDefaultInfo(tmpJson,in);
	}
	return retStr;
}


string getReply_oprional(Json::Value& root)
{
	ISMaster::InParas in;
	string retStr;
	if(!root["reply_optional"].isNull())
	{
		Json::Value tmpJson=root["reply_optional"];
		retStr=readDefaultInfo(tmpJson,in);
	}
	return retStr;
}

string getReply_Y(Json::Value& root)
{
	ISMaster::InParas in;
	string retStr;
	if(!root["reply_Y"].isNull())
	{
		Json::Value tmpJson=root["reply_Y"];
		retStr=readDefaultInfo(tmpJson,in);
	}
	return retStr;
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

bool MakeInParasProto(std::string sQuery,std::string userId,int actionType, std::string &sOut)
{
	ISMaster::InParas paras;
	ISMaster::Userdata* pUserData = paras.mutable_userinfo();
	paras.set_tid("1000000001");
	pUserData->set_userid(userId);
	//ISMaster::Business *p
	// 1 NETWORKSTANDRD::NET_4G
	// 2 CHANNELTYPE::SHORTMESSAGE
	pUserData->set_networkstandard(NETWORKSTANDRD_NET_2G);
	pUserData->set_channeltype(CHANNELTYPE_SHORTMESSAGE);
	pUserData->set_brand("cos");
	pUserData->set_city("hefei");
	paras.set_text(sQuery);
	//paras.set_preprocesstype(7);
	paras.set_actiontype((ISMaster::ACTIONTYPE)actionType);
	//cout << paras.Utf8DebugString() <<endl;
	return paras.SerializeToString(&sOut);
}


int WorkingFunc(string & sConfig,Json::Value &tmpJson,int loopNum)
{
	if ( 0 != Init(sConfig.c_str()))
	{
		cout << "初始化失败!" <<endl;
		return -1;
	}
	
	int nCount = 0;
	do 
	{
		nCount++;
		std::string sOut;
		char szOut[20480] = {0};
		int nOut = 0;
		string protobuf;
		
		INFO("***************第"<<nCount<<"次循环***************");
		protobuf=getDelSession(tmpJson);
		ISMaster::InParas in;
		in.ParseFromString(protobuf);
		INFO("***DelSession输入信息\n***"<<in.Utf8DebugString());
		if ( 0 != Query(protobuf.c_str(),szOut,&nOut))
		{
			cout << "Query失败!" <<endl;
			continue;
		}
		ISMaster::SHNote oResult;
		sOut.clear();
		sOut.append(szOut,nOut);
		oResult.ParseFromString(sOut);
		INFO("***DelSession输出信息\n***"<<oResult.Utf8DebugString());
		

		protobuf=getOrderLiuLiang(tmpJson);
		in.ParseFromString(protobuf);
		INFO("***OrderLiuLiang输入信息\n***"<<in.Utf8DebugString());
		if ( 0 != Query(protobuf.c_str(),szOut,&nOut))
		{
			cout << "Query失败!" <<endl;
			continue;
		}
		sOut.clear();
		sOut.append(szOut,nOut);
		oResult.ParseFromString(sOut);
		INFO("***OrderLiuLiang输出信息\n***"<<oResult.Utf8DebugString());
			
		
		protobuf=getDaoHang(tmpJson);
		in.ParseFromString(protobuf);
		INFO("***DaoHang输入信息\n***"<<in.Utf8DebugString());
		if ( 0 != Query(protobuf.c_str(),szOut,&nOut))
		{
			cout << "Query失败!" <<endl;
			continue;
		}
		sOut.clear();
		sOut.append(szOut,nOut);
		oResult.ParseFromString(sOut);
		INFO("***DaoHang输出信息\n***"<<oResult.Utf8DebugString());
		
		
		protobuf=getReply_oprional(tmpJson);
		in.ParseFromString(protobuf);
		INFO("***Reply_optional输入信息\n***"<<in.Utf8DebugString());
		if ( 0 != Query(protobuf.c_str(),szOut,&nOut))
		{
			cout << "Query失败!" <<endl;
			continue;
		}
		sOut.clear();
		sOut.append(szOut,nOut);
		oResult.ParseFromString(sOut);
		INFO("***Reply_optional输出信息\n***"<<oResult.Utf8DebugString());
		
		
		protobuf=getReply_Y(tmpJson);
		in.ParseFromString(protobuf);
		INFO("***Reply_Y输入信息\n***"<<in.Utf8DebugString());
		if ( 0 != Query(protobuf.c_str(),szOut,&nOut))
		{
			cout << "Query失败!" <<endl;
			continue;
		}
		sOut.clear();
		sOut.append(szOut,nOut);
		oResult.ParseFromString(sOut);
		INFO("***Reply_Y输出信息\n***"<<oResult.Utf8DebugString());
		
	} while ( nCount < loopNum);
	
	return 0;
}

int Test_WorkingFunc(std::string sQuery,int actionType,int loopNum)
{
	int nCount = 0;
	do 
	{
		std::string sIn;
		std::string sUserId = "13666668888";
		if( !MakeInParasProto(sQuery,sUserId,actionType,sIn))
		{
			ERROR("构造输入错误!");
			continue;
		}
		std::string sOut;
		char szOut[2048] = {0};
		int nOut = 0;
		if ( 0 != Query(sIn.c_str(),szOut,&nOut))
		{
			ERROR("结果错误!");
			continue;
		}
		ISMaster::SHNote oResult;
		sOut.append(szOut,nOut);
		oResult.ParseFromString(sOut);
		cout << "--------------------------------------------------" <<endl;
		cout <<"输出结果:" << endl << oResult.Utf8DebugString() << endl;
		cout << "--------------------------------------------------" <<endl;
		++nCount;
		DEBUG("第 " << nCount << " 次查询处理完成!");

		//清理
		//sIn.clear();
		//if( !MakeInParasProto(sQuery,sUserId,ISMaster::ACTIONTYPE_CLEAR,sIn))
		//{
		//	continue;
		//}
		//if ( 0 != Query(sIn.c_str(),szOut,&nOut))
		//{
		//	ERROR("结果错误!");
		//	continue;
		//}
		//sOut.clear();
		//sOut.append(szOut,nOut);
		//oResult.ParseFromString(sOut);
		//cout << "--------------------------------------------------" <<endl;
		////cout <<"输出结果:" << endl << oResult.Utf8DebugString() << endl;
		//cout << "--------------------------------------------------" <<endl;
		//DEBUG("第 " << nCount << " 次清理处理完成!");

	} while ( nCount < loopNum);

	return 0;
}



string trim(const string& str)  //delete the space of front and tail
{
    string::size_type pos = str.find_first_not_of(' ');
    if (pos == string::npos)
    {
        return str;
    } 
    string::size_type pos2 = str.find_last_not_of(' ');
    if (pos2 != string::npos)
    {
        return str.substr(pos, pos2 - pos + 1);
    }
    return str.substr(pos);
}


void split(string str,vector<string>& res)
{
    if(str.empty())
    {
        return;
    }
    string ss="";
    for(int i=0;i<=str.size();++i)
    {
        if(str[i]==' ' || str[i]=='\0')
        {
            if(!ss.empty())
            {
                res.push_back(ss);
                ss="";
            }
        }
        else
        {
            ss+=str[i];
        }

    }
}

string deln(string str)
{
	string nstr="";
	//cout << "----------------str:" << str << endl;
	for(int i=0;i<str.size();++i)
	{
		if(str[i]=='\r' || str[i]=='\n')
		{
			continue;
		}
		nstr += str[i];
	}
	
	//cout << "----------------nstr:" << nstr << endl;
	return nstr;
}

















