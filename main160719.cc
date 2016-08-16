#include <iostream>
//#include "ICEClient.h"
#include <stdio.h>
#include <vector>
#include <string.h>
//#include <dlfcn.h>
//#include <math.h>
#include <fstream>
#include <sstream>
#include <time.h>
#include "ISMasterApi.h"
#include "IntelAssitant.pb.h"

#if WIN32
#include <Windows.h>
#endif

//#include "loggerWrapper.h"
//2016-07-19am

using namespace std;

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

void ConvertGBKToUtf8(std::string& amp, std::string strGBK);
void ConvertUtf8ToGBK(std::string&amp, std::string strUtf8);
char *getFileContents(const char *fileName,int *fileLength);
bool MakeInParasProto(std::string sIn,std::string &sOut);
bool MakeInParasProto(std::string sIn,std::string &sOut,vector<std::string> &strs);
void split(string str,vector<string>& res);
string trim(const string& str);
string deln(string str);

int main()
{
	
	int count=0;
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
	//read input.txt 
	std::string line="";
	vector<std::string> instrs;
	vector<std::string> outstrs;
	ifstream infile("input.txt");
	if(!infile.is_open())
	{
		cout<<"Error opening 'input.txt'"<<endl;
		exit(-1);
	}
	while(getline(infile,line))
	{
		//cout<<line<<endl;
		line=deln(line);
		instrs.push_back(line);
		//outstrs.push_back(line);
	}
	cout<<instrs.size()<<"-------------------------------------------size"<<endl;
	
	infile.close();
	
	
	for(int i=0;i<instrs.size();++i)
	{
		clock_t start_time=clock();
		//std::string sPreIn = "开通全球通4G10元50M流量"; //此处随意输入一段字符作为测试使用
		//std::string sPreIn = instrs[i];
		//cout<<instrs[i]<<" ---------------------------instrs[i]&sPreIn"<<endl;
		std::string sPreOut = "";
		int type = (0x0001 | 0x0002);      //------------------?

		if ( 0 != PreProcess(instrs[i].c_str(),(char *)sPreOut.c_str(),type))  //通过preProcessP这个指针调用函数 PreProcess(const char*)
		{
			cout <<  "PreProcess failed in line : " <<i<<endl;
			sPreOut = "PreProcess Failed  ";
			return -1;
		}
		//std::string index = std::to_string(i); 
		clock_t end_time=clock();
		double td=static_cast<double>(end_time-start_time)/CLOCKS_PER_SEC*1000;
		string timestr;
		stringstream ss;
		ss<<td;
		ss>>timestr;
		sPreOut=deln(sPreOut);
		instrs[i]=deln(instrs[i]);
		outstrs.push_back(timestr+"  "+instrs[i]+" "+sPreOut);
		//cout<<outstrs[i]<<"-------------   ---preprocess test"<<endl;
		count++;
		//cout<<"run : "<<count<<" times"<<endl;
	}	
	
	
	//------------------------------------------------------------------
	//Query Funciton testing
	// read queryInput.txt 
	vector<std::string> orgStr;
	vector<vector<std::string> > qIn;//restore the inparas structrue data
	vector<std::string> qOut;
	vector<std::string> tmp; //used to split one line to several parameters temperary
	//vector<std::string> outstrs;
	ifstream queryInfile("queryInput.txt");
	std::string qline="";
	if(!queryInfile.is_open())
	{
		cout<<"Error opening 'QueryInput.txt'"<<endl;
		exit(-1);
	}
	else{
		cout<<"-----------------------------reading queryInput.txt "<<endl;
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
	cout<< "--------------------------reading queryInput.txt finished! "<<endl; 
	
	std::string sIn="123456";
	std::string sQueryText("开通全球通10元流量包");
	//std::string sQueryText(instrs[0]);
	//INFO(sQueryText);
	for(int i=0;i<qIn.size();++i)
	{
		//clock_t start_time2=clock();
		if( !MakeInParasProto(sQueryText,sIn,qIn[i]))    //use this Function ,sIn will become an binary string
		{
			cout<<"fail to make InParasProto !"<<endl;
			return -1;
		}
		std::string sOut;
		char szOut[10240] = {0};
		int nOut = 0;
		std::string sop;
		if ( 0 != Query(sIn.c_str(),szOut,&nOut))
		{
			cout<<"Query Function failed"<<endl;
			//return -1;
			sop = "Query Function failed";
			
		}
		else
		{
			ISMaster::OutParas oResult;
			sOut.append(szOut,nOut);
			oResult.ParseFromString(sOut);
			//INFO(oResult.Utf8DebugString());
			cout << "-----------------get OutParas-------     ---------" <<endl;
			cout << oResult.Utf8DebugString() << endl;
			cout << "--------------------------------------------------" <<endl;
			sop = oResult.Utf8DebugString();   //record the result of Query()
		}
		
		//clock_t end_time2=clock();
		//double td=static_cast<double>(end_time2-start_time2)/CLOCKS_PER_SEC*1000;
		//string timestr="";
		//stringstream ss;
		//ss<<td;
		//ss>>timestr;
		
		sop=deln(sop);
		orgStr[i]=deln(orgStr[i]);
		qOut.push_back("   "+orgStr[i]+"  "+sop);
		
	}
	

	
	//---------------------------------------------------------------------------
	//Uninit Function Testing
	if(0!=Uninit())
	{
		cout<<"fail to Uninit !"<<endl;
		return -1;
	}
	
	
	// make output.txt
	ofstream outfile("output.txt",ios::out);
	if(!outfile)
	{
		cerr<<"fail to open file 'output.txt'"<<endl;
		return -1;
	}
	for(int i=0;i<outstrs.size();++i)
	{
		outfile<<outstrs[i]<<endl;
	}
	outfile.close();
	
	//make queryOutput.txt
	ofstream queryOutfile("queryOutput.txt",ios::out);
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
	
	cout<<"total run : "<<count<<"  times"<<endl;
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

bool MakeInParasProto(std::string sIn,std::string &sOut,vector<std::string> &strs)
{
	ISMaster::InParas paras;
	cInparas cIn;
	cIn.vec2class(strs);
	cIn.printClass();
	
	paras.set_tid(cIn.tid); //("10000001");
	paras.set_userid(cIn.userid); //("200001231");
	paras.set_text(cIn.text); //(sIn);
	// 1 NETWORKSTANDRD::NET_4G
	// 2 CHANNELTYPE::SHORTMESSAGE
	paras.set_priority(cIn.priority);
	paras.set_networkstandard(cIn.networkstandard); 
	paras.set_channeltype(cIn.channeltype);
	//paras.set_preprocesstype(7);
	paras.set_actiontype(cIn.actiontype);  //("normal");
	
	//Business exbusiness
	
	Business *pBs = paras.add_exbusiness();
	pBs->set_name("全球通");
	pBs->set_status(1);
	pBs->set_price(58);
	pBs->set_expiretime("2019-12-31");
	pBs->set_starttime("2015-12-31");
	
	/*
	cout<<" ----------------------information in InParas----------------"<<endl;
	cout<<paras.tid()<<"   "<<paras.userid()<<"    "<<paras.networkstandard()<<"    ";
	cout<<paras.channeltype()<<"   "<<paras.text()<<"   "<<paras.actiontype()<<endl;
	*/
	cout<< "----------------------get binary file 'InParas'"<<endl;
	cout << paras.Utf8DebugString() <<endl;
	cout<<"------------------------------------------------"<<endl;
	return paras.SerializeToString(&sOut);
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

