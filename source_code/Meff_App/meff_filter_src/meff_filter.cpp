#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include <vector>
#include <set>
#include <algorithm>
#include <omp.h>
#include <time.h>
#include <getopt.h>
#include "mt19937ar.h"
#include "cdhit_seqdb.h"
using namespace std;


//-------- utility ------//
void getBaseName(string &in,string &out,char slash,char dot)
{
	int i,j;
	int len=(int)in.length();
	for(i=len-1;i>=0;i--)
	{
		if(in[i]==slash)break;
	}
	i++;
	for(j=len-1;j>=0;j--)
	{
		if(in[j]==dot)break;
	}
	if(j==-1)j=len;
	out=in.substr(i,j-i);
}
void getRootName(string &in,string &out,char slash)
{
	int i;
	int len=(int)in.length();
	for(i=len-1;i>=0;i--)
	{
		if(in[i]==slash)break;
	}
	if(i<=0)out=".";
	else out=in.substr(0,i);
}

//=================== upper and lower case ====================//
//----------upper_case-----------//
void toUpperCase(char *buffer)
{
	for(int i=0;i<(int)strlen(buffer);i++)
	if(buffer[i]>=97 && buffer[i]<=122) buffer[i]-=32;
}
void toUpperCase(string &buffer)
{
	for(int i=0;i<(int)buffer.length();i++)
	if(buffer[i]>=97 && buffer[i]<=122) buffer[i]-=32;
}
//----------lower_case-----------//
void toLowerCase(char *buffer)
{
	for(int i=0;i<(int)strlen(buffer);i++)
	if(buffer[i]>=65 && buffer[i]<=90) buffer[i]+=32;
}
void toLowerCase(string &buffer)
{
	for(int i=0;i<(int)buffer.length();i++)
	if(buffer[i]>=65 && buffer[i]<=90) buffer[i]+=32;
}

//----- get upper case -----//
int getUpperCase(char *buffer)
{
	int count=0;
	for(int i=0;i<(int)strlen(buffer);i++)
	if(buffer[i]>=65 && buffer[i]<=90) count++;
	return count;
}
int getUpperCase(string &buffer)
{
	int count=0;
	for(int i=0;i<(int)buffer.length();i++)
	if(buffer[i]>=65 && buffer[i]<=90) count++;
	return count;
}
//----- get lower case -----//
int getLowerCase(char *buffer)
{
	int count=0;
	for(int i=0;i<(int)strlen(buffer);i++)
	if(buffer[i]>=97 && buffer[i]<=122) count++;
	return count;
}
int getLowerCase(string &buffer)
{
	int count=0;
	for(int i=0;i<(int)buffer.length();i++)
	if(buffer[i]>=97 && buffer[i]<=122) count++;
	return count;
}


//------------ load MSA in A2M --------------//
long Load_MSA_in_A2M(string &multi_fasta,vector <string> &nam_list,vector <string> &fasta_list, int &totlen)
{
	ifstream fin;
	string buf,temp;
	//-> 1. read
	fin.open(multi_fasta.c_str(), ios::in);
	if(fin.fail()!=0)
	{
		fprintf(stderr,"file %s not found!\n",multi_fasta.c_str());
		exit(-1);
	}
	//-> 2. load
	int first=1;
	int length=0;
	long count=0;
	char command[300000];
	nam_list.clear();
	fasta_list.clear();
	for(;;)
	{
		if(!getline(fin,buf,'\n'))break;
		sprintf(command,"%ld",count);
		string temp=command;
		nam_list.push_back(temp);
		fasta_list.push_back(buf);
		count++;
		//length check
		if(first==1)
		{
			first=0;
			length=(int)buf.length();
		}
		else
		{
			if(length!=(int)buf.length())
			{
				fprintf(stderr,"ini_length %d not equal to cur_length %d at row %ld \n",
					length,(int)buf.length(),count);
				exit(-1);
			}
		}
	}
	//-> 3. return
	totlen=length;
	return count;
}



//-------- read in MSA in a3m format (i.e., normal FASTA with upper/lower) ------------//
//[note]: we set the first sequence as the query sequence,
//        that is to say, all the following sequences should be longer than the first
int Multi_FASTA_Input(string &multi_fasta,vector <string> &nam_list,vector <string> &fasta_list)
{
	ifstream fin;
	string buf,temp;
	//read
	fin.open(multi_fasta.c_str(), ios::in);
	if(fin.fail()!=0)
	{
		fprintf(stderr,"file %s not found!\n",multi_fasta.c_str());
		return -1;
	}
	//load
	int firstlen=0;
	int first=1;
	int count=0;
	int number=0;
	string name;
	string seq;
	nam_list.clear();
	fasta_list.clear();
	for(;;)
	{
		if(!getline(fin,buf,'\n'))break;
		if(buf=="")continue;
		if(buf.length()>=1 && buf[0]=='>')
		{
			name=buf.substr(1,buf.length()-1);
			nam_list.push_back(name);
			count++;
			if(first!=1)
			{
				fasta_list.push_back(seq);
				number++;
				if(number==1)
				{
					firstlen=(int)seq.length();
				}
				else
				{
					int lowlen=getLowerCase(seq);
					int curlen=(int)seq.length()-lowlen;
					if(curlen!=firstlen)
					{
						fprintf(stderr,"length not equal at %s, [%d!=%d] \n",buf.c_str(),curlen,firstlen);
						return -1;
					}
				}
			}
			first=0;
			seq="";
		}
		else
		{
			if(first!=1)seq+=buf;
		}
	}
	//final
	if(first!=1)
	{
		fasta_list.push_back(seq);
		number++;
		if(number==1)
		{
			firstlen=(int)seq.length();
		}
		else
		{
			int lowlen=getLowerCase(seq);
			int curlen=(int)seq.length()-lowlen;
			if(curlen!=firstlen)
			{
				fprintf(stderr,"length not equal at %s, [%d!=%d] \n",buf.c_str(),curlen,firstlen);
				return -1;
			}
		}
	}
	//check
	if(number!=count)
	{
		fprintf(stderr,"num %d != count %d \n",number,count);
		return -1;
	}
	return count;
}

//----- eliminate lower case -------//
void Eliminate_LowerCase(string &instr,string &outstr)
{
	int i;
	outstr.clear();
	for(i=0;i<(int)instr.length();i++)
	{
		if(instr[i]>='a' && instr[i]<='z') continue;
		outstr.push_back(instr[i]);
	}
}

//----- eliminate head/tail gap on query sequence ----//
//-> [note]: we ALWAYS fix the query sequence as the 1st one
void Eliminate_HeadTail_Gap(vector <string> &fasta_list)
{
	int i,k;
	int len=(int)fasta_list[0].length();
	int size=(int)fasta_list.size();
	//-> determine start
	int start=-1;
	for(i=0;i<len;i++)
	{
		if(fasta_list[0][i]!='-')
		{
			start=i;
			break;
		}
	}
	//-> determine end
	int end=-1;
	for(i=len-1;i>=0;i--)
	{
		if(fasta_list[0][i]!='-')
		{
			end=i;
			break;
		}
	}
	//-> judge
	if(start==-1 || end==-1)
	{
		fprintf(stderr,"start %d or end %d not correct \n",start,end);
		exit(-1);
	}
	//-> cut head and tail
	if(start !=0 || end !=len-1)
	{
		for(k=0;k<size;k++)fasta_list[i]=fasta_list[i].substr(start,end-start+1);
	}
}


//========= validate sequence ==========//
int Ori_AA_Map[26]=
{ 0,20,2,3,4,5,6,7,8,20,10,11,12,13,20,15,16,17,18,19,20, 1, 9,20,14,20};
// A B C D E F G H I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
// 0 1 2 3 4 5 6 7 8  9 10 11 12 14 14 15 16 17 18 19 20 21 22 23 24 25

void Validate_Sequence(string &instr,string &outstr)
{
	int i;
	int len=(int)instr.length();
	outstr=instr;
	for(i=0;i<len;i++)
	{
		if(instr[i]=='-')continue;
		char a=instr[i];
		if(a<'A' || a>='Z')
		{
			outstr[i]='X';
			continue;
		}
		int retv=Ori_AA_Map[a-'A'];
		if(retv==20)
		{
			outstr[i]='X';
			continue;
		}
	}
}


//===================== for CDHIT preparation =====================//

//---------- remove gap ------------//
void Remove_Gap(string &in,string &out)
{
	int i;
	int len=(int)in.length();
	out="";
	for(i=0;i<len;i++)
	{
		if(in[i]!='-')out+=in[i];
	}
}

//------------ A2M to non-gap seq ------------//
void A2M_To_NonGapSEQ(vector <string> &in, vector <string> &out)
{
	long i;
	long len=(int)in.size();
	out=in;
	for(i=0;i<len;i++)
	{
		Remove_Gap(in[i],out[i]);
	}
}

//-------- extract CD-HIT fragment --------//
long CD_HIT_StrProc(string &in_str,int &start,int &end,
	long totnum,int totlen)
{
	int i;
	int cur;
	long seq_id;
	string temp;
	int len=(int)in_str.length();
	//get seq_id
	for(i=0;i<len;i++)
	{
		if(in_str[i]==':')break;
	}
	temp=in_str.substr(0,i);
	seq_id=atol(temp.c_str());
	if(seq_id<0 || seq_id>=totnum)return -1;
	//get start
	cur=i+1;
	for(i=cur;i<len;i++)
	{
		if(in_str[i]=='-')break;
	}
	temp=in_str.substr(cur,i-cur);
	start=atoi(temp.c_str());
	if(start<0 || start>=totlen)return -1;
	//get end
	cur=i+1;
	temp=in_str.substr(cur,len-cur);
	end=atoi(temp.c_str());
	if(end<0 || end>=totlen)return -1;
	//return
	return seq_id;
}

//------------- CD-HIT process ---------------//
//[note]: sim_thres should be set to 0.7
//        len_thres should be set to 0 (i.e., no restriction)
//[output]: all clusters with one or more than one members
int CD_HIT_Process_New(	
	vector <string> &out_nam,                           //-> input, OriginID/Range
	vector <string> &out_seq,                           //-> input, ungapped fragments
	double sim_thres,double len_thres,
	vector <vector <string> > & output, vector <int> & center,
	int cpu_num)
{
	//-> 0. run CD-HIT
//	int sys_retv;
//	char command[30000];
//	sprintf(command,"./cd-hit -i %s -o %s -c %lf -s %lf -M 0 -T 0 -d 0 1> ws1 2> ws2",
//		infile.c_str(),outname.c_str(),sim_thres,len_thres);
//	sys_retv=system(command);

	//-> 1. initial CDHIT
	Options options;
	SequenceDB seq_db;
	options.cluster_thd=sim_thres;   // -c sim_thres
	options.diff_cutoff=len_thres;   // -s len_thres
	options.max_memory=0;            // -M 0
	options.des_len=0;               // -d 0

#ifndef NO_OPENMP
	int cpu = omp_get_num_procs();   // -T 0
	if(cpu_num<=0)options.threads = cpu;
	else
	{
		int rel_cpu=cpu_num<cpu?cpu_num:cpu;
		options.threads = rel_cpu;
	}
#endif

	options.Validate();
	InitNAA( MAX_UAA );
	options.NAAN = NAAN_array[options.NAA];
	seq_db.NAAN = NAAN_array[options.NAA];

	//-> 2. run CDHIT to get all clusters with more than one members
	seq_db.Read_From_String( options, out_nam, out_seq );
	seq_db.SortDivide( options );
	seq_db.DoClustering( options );
	seq_db.WriteExtra1D_To_String( options, output,center);

	//return
	CDHIT_CleanUpTempFiles();
	return (int)center.size();
}

//--------- from CD-HIT cluster, construct a cluster matrix ---------//
void Cluster_To_Matrix(vector <vector <string> > & strnam, vector <int> & center,
		map<long, int > & cluster_mapping, long input_size, int totlen )
{
	map<long, int >::iterator iter;
	cluster_mapping.clear();
	int i,k,l;
	int cluster_size=(int)center.size();
	//-> process each cluster
	for(i=0;i<cluster_size;i++)
	{
		int member=(int)strnam[i].size();
		//-> process pairwise cluster mapping
		for(k=0;k<member;k++)
			for(l=0;l<member;l++)
			{
				if(k==l)continue;
				long seqid_k=atol(strnam[i][k].c_str());
				long seqid_l=atol(strnam[i][l].c_str());
				//generate uniq_ID
				long uniq_ID=input_size*seqid_k+seqid_l;
				//add to map
				iter = cluster_mapping.find(uniq_ID);
				if(iter != cluster_mapping.end())continue;
				cluster_mapping.insert(map < long, int >::value_type(uniq_ID, i));
			}
	}
}


//================== original Meff calculation ================//
void compute_similarity(const string &a, const string &b, double &sim1, double &sim2)
{
	if(a.length() != b.length() )
		exit(-2);
	int len = 0, match=0;
	int len1 = 0, len2 = 0;
	for(size_t i=0; i<a.length(); ++i)
	{
		if(a[i] != '-')len1++;
		if(b[i] != '-')len2++;
		if(a[i] == '-' || b[i] == '-')
			continue;
		if(a[i] == b[i])
			++ match;
		++ len;
	}
	sim1=1.0*match/len1;
	sim2=1.0*match/len2;
}


//====================== binary search =====================//

// A recursive binary search function. It returns 
// location of x in given array arr[l..r] is present, 
// otherwise -1
long binarySearch_recur(vector <double> &arr, long l, long r, double x, long size)
{
	if (r >= l)
	{
		long mid = l + (r - l)/2;
		long nxt = mid+1;
		if(nxt>=size)
			return mid;

		// If the element is present at the middle 
		// itself
		if ( arr[mid]<=x && x<=arr[nxt])
			return mid;

		// If element is smaller than mid, then 
		// it can only be present in left subarray
		else if ( arr[mid] > x) 
			return binarySearch_recur(arr, l, mid-1, x, size);

		// Else the element can only be present
		// in right subarray
		else
			return binarySearch_recur(arr, mid+1, r, x, size);
	}
	
	// We reach here when element is not 
	// present in array
	return 0;
}


// A iterative binary search function. It returns
// location of x in given array arr[l..r] if present,
// otherwise -1
long binarySearch_iter(vector <double> &arr, long l, long r, double x, long size)
{
	while (l <= r)
	{
		long mid = l + (r - l)/2;
		long nxt = mid+1;
		if(nxt>=size)
			return mid;

		// Check if x is present at mid
		if ( arr[mid]<=x && x<=arr[nxt])
			return mid;
		
		// If x greater, ignore left half
		else if (arr[mid] < x)
			l = mid + 1;
		
		// If x is smaller, ignore right half
		else
			r = mid - 1;
	}
 
	// if we reach here, then element was
	// not present
	return 0;
}



//---------- usage ---------//
void Usage() 
{
	fprintf(stderr,"Version: 1.03 \n");
	fprintf(stderr,"Meff_Filter -i/I a3m(a2m)_input -o/O a3m(a2m)_output [-s sim_thres] [-c cut_num] [-C cpu_num] \n");
	fprintf(stderr,"           [-f first] [-S cdhit_thres] [-n sample_num] [-d identity] [-v verbose] [-r rand_seed] \n");
	fprintf(stderr,"Usage : \n\n");
	fprintf(stderr,"-i/I aXm_input :  Input MSA file in A3M/A2M format. \n\n");
	fprintf(stderr,"-o/O aXm_output : Output filterd MSA file in A3M/A2M format. \n\n");
	fprintf(stderr,"-s sim_thres :    Similarity threshold to calculate Meff. \n");
	fprintf(stderr,"                  (by default, sim_thres = 0.7, should between 0.7 to 1.0) \n\n");
	fprintf(stderr,"-c cut_num :      If seq_num in MSA > cut_num, then call CD-HIT. \n");
	fprintf(stderr,"                  (by default, cut_num = 20000, set -1 to disable CD-HIT) \n\n");
	fprintf(stderr,"-C cpu_num :      CPU number. [default = -1 to use ALL] \n\n");
	fprintf(stderr,"-f first :        Pivot sequence position. [default: 0 (0-base)] \n\n");
	fprintf(stderr,"-S cdhit_thres :  Similarity threshold to run CD-HIT. \n");
	fprintf(stderr,"                  (by default, cdhit_thres = 0.65, should between 0.65 to 1.0) \n\n");
	fprintf(stderr,"-n sample_num :   Sample number to produce a new MSA (default: 20000) \n\n");
	fprintf(stderr,"-d identity:      Consider sequence identity in sampleing (default: 0 for NO) \n\n");
	fprintf(stderr,"-v verbose :      Verbose running stage or not (default: 0 for NO verbose) \n\n");
	fprintf(stderr,"-r rand_seed :    Random seed (default: -1 for time(NULL) ) \n\n");
}

//------------ main ---------------//
int main(int argc, char *argv[])
{
	//------ Calculate MEFF (cdhit version) -------//
	if(argc<3)
	{
		Usage();
		exit(-1);
	}
	string axm_input="";
	string axm_output="";
	int axm_input_key=0;
	int axm_output_key=0;
	double sim_thres=0.7;      //-> Similarity threshold to calculate Meff (default: 0.7)
	int cut_num=20000;         //-> Sequence cut number to call CD-HIT (default: 20000)
	int cpu_num=-1;            //-> default is -1 to use ALL cpus.
	int first_seq=0;           //-> by default, we put the first sequence as the pivot sequence.
	double cdhit_thres=0.65;   //-> Similarity threshold to run CD-HIT (default: 0.65)
	int verbose=0;             //-> Verbose CD-HIT stage or not (default: 0)
	long seed=-1;              //-> -1 for using time(NULL) to generate the seed.
	//--- for sample purpose --//
	long sample_num=20000;
	int identity=0;

	//command-line arguments process
	extern char* optarg;
	char c = 0;
	while ((c = getopt(argc, argv, "i:I:o:O:s:c:C:f:S:n:d:v:r:")) != EOF) {
		switch (c) {
		case 'i':
			axm_input = optarg;
			axm_input_key = 0;  //-> 0 for a3m
			break;
		case 'I':
			axm_input = optarg;
			axm_input_key = 1;  //-> 1 for a2m
			break;
		case 'o':
			axm_output = optarg;
			axm_output_key = 0; //-> 0 for a3m
			break;
		case 'O':
			axm_output = optarg;
			axm_output_key = 1; //-> 1 for a2m
			break;
		case 's':
			sim_thres = atof(optarg);
			break;
		case 'c':
			cut_num = atoi(optarg);
			break;
		case 'C':
			cpu_num = atoi(optarg);
			break;
		case 'f':
			first_seq = atoi(optarg);
			break;
		case 'S':
			cdhit_thres = atof(optarg);
			break;
		case 'n':
			sample_num = atoi(optarg);
			break;
		case 'd':
			identity = atoi(optarg);
			break;
		case 'v':
			verbose = atoi(optarg);
			break;
		case 'r':
			seed = atol(optarg);
			break;
		default:
			Usage();
			exit(-1);
		}
	}

	//check arguments
	if(axm_input=="")
	{
		fprintf(stderr,"a3m(a2m)_input should be specified. \n");
		exit(-1);
	}
	if(axm_output=="")
	{
		fprintf(stderr,"a3m(a2m)_output should be specified. \n");
		exit(-1);
	}
	if( sim_thres<0.7 || sim_thres>1  )
	{
		fprintf(stderr,"sim_thres %lf should between 0.7 and 1.0 \n",sim_thres);
		exit(-1);
	}
	if( cdhit_thres<0.65 || cdhit_thres>1  )
	{
		fprintf(stderr,"cdhit_thres %lf should between 0.65 and 1.0 \n",sim_thres);
		exit(-1);
	}

#ifndef NO_OPENMP
	int CPU_NUM = omp_get_num_procs();   // -T 0
	if(cpu_num<=0)omp_set_num_threads(CPU_NUM);
	else
	{
		int rel_cpu=cpu_num<CPU_NUM?cpu_num:CPU_NUM;
		omp_set_num_threads(rel_cpu);
	}
#endif

	//----- general data structure ---//
	vector <string> nam_list;
	vector <string> fasta_list;
	vector <string> fasta_list_orig;
	long msa_num;
	int totlen;


	//----- load input A3M -------//
	if(axm_input_key==0)
	{
		msa_num=Multi_FASTA_Input(axm_input,nam_list,fasta_list_orig);
		//length check
		int first=1;
		for(long i=0;i<msa_num;i++)
		{
			//get sequence
			string seq;
			Eliminate_LowerCase(fasta_list_orig[i],seq);
			string outseq;
			Validate_Sequence(seq,outseq);
			//length check
			if(first==1)
			{
				totlen=(int)outseq.length();
				first=0;
			}
			else
			{
				if(totlen!=(int)outseq.length())
				{
					fprintf(stderr,"ini_length %d not equal to cur_length %d at pos %ld \n",
						totlen,(int)outseq.length(),i);
					exit(-1);
				}
			}
			//push back
			fasta_list.push_back(outseq);
		}
	}
	//----- load input A2M -------//
	else
	{
		msa_num=Load_MSA_in_A2M(axm_input,nam_list,fasta_list,totlen);
		fasta_list_orig=fasta_list;
	}
	//----- eliminate head/tail gap ----//
	Eliminate_HeadTail_Gap(fasta_list);


	//------ use CD-HIT or not ----//
	vector<int> sim(msa_num, 0);
	if(msa_num>cut_num && cut_num>=0) //-> use CD-HIT if msa_nun > cut_num (default: 20000)
	{
		//-> cd-hit pre-process
		//--| remove gap in the input sequences
		vector <string> fasta_seq=fasta_list;
		A2M_To_NonGapSEQ(fasta_list, fasta_seq);
		//--| use a unique ID for input names (starting from 0)
		vector <string> out_nam=nam_list;
		if(axm_input_key==0)
		{
			#pragma omp for schedule(dynamic)
			for(long i=0;i<msa_num;i++)
			{
				stringstream ss;
				ss << i;
				out_nam[i] = ss.str();
			}
		}
		//-> run cd-hit to generate cluster_matrix
		double sim_thres_cdhit=cdhit_thres;
		double len_thres=0;
		vector <vector <string> > strnam;
		vector <int> center;

if(verbose==1)fprintf(stderr,"cdhit process\n");
		CD_HIT_Process_New(out_nam,fasta_seq,sim_thres_cdhit,len_thres,
			strnam,center,cpu_num);

if(verbose==1)fprintf(stderr,"cluster to matrix\n");
		map<long, int > cluster_mapping;
		Cluster_To_Matrix(strnam,center,cluster_mapping,msa_num,totlen);

if(verbose==1)fprintf(stderr,"calculate meff\n");
		//-> perform normal meff calculation
		#pragma omp parallel
		{
			vector<int> sim_local(msa_num, 0);
			//-> perform normal meff calculation by the aid of cluster_matrix
			#pragma omp for schedule(dynamic)
			for(long i=0; i<msa_num; ++i)
				for(long j=i+1; j<msa_num; ++j)
				{
					//--| check from cluster_matrix
					map<long, int >::iterator iter;
					long uniq_ID=msa_num*i+j;
					iter = cluster_mapping.find(uniq_ID);
					if(iter == cluster_mapping.end())continue;
					//--| calculate Meff
					double sim1,sim2;
					compute_similarity(fasta_list[i], fasta_list[j],sim1,sim2);
					if( sim1 > sim_thres )sim_local[i] ++;
					if( sim2 > sim_thres )sim_local[j] ++;
				}
			#pragma omp critical
			{
				for(long i=0; i<msa_num; ++i)sim[i]+=sim_local[i];
			}
		}
	}
	else
	{

if(verbose==1)fprintf(stderr,"calculate meff\n");
		//-> perform normal meff calculation
		#pragma omp parallel
		{
			vector<int> sim_local(msa_num, 0);
			#pragma omp for schedule(dynamic)
			for(long i=0; i<msa_num; ++i)
				for(long j=i+1; j<msa_num; ++j)
				{
					//--| calculate Meff
					double sim1,sim2;
					compute_similarity(fasta_list[i], fasta_list[j],sim1,sim2);
					if( sim1 > sim_thres )sim_local[i] ++;
					if( sim2 > sim_thres )sim_local[j] ++;
				}
			#pragma omp critical
			{
				for(long i=0; i<msa_num; ++i)sim[i]+=sim_local[i];
			}
		}
	}


if(verbose==1)fprintf(stderr,"accumulate distribution\n");

	//=============== we use the generated MEFF matrix to sample =================//
	//------ calculate identity -------//
	vector <double> idweight(msa_num,1);
	if(identity==1)
	{
		#pragma omp parallel for schedule(dynamic)
		for(long i=0; i<msa_num; ++i)
		{
			double sim1,sim2;
			compute_similarity(fasta_list[0], fasta_list[i],sim1,sim2);
			idweight[i]=sim1;
		}
	}

	//------ final filter -------//
	//-> step 1
	//   (calculate sequecne weight)
	vector <double> weight(msa_num,0);
	double total_weight=0;
	for(long i=0; i<msa_num; ++i)
	{
		weight[i]= (1.0/(sim[i]+1))*idweight[i];
		total_weight+=weight[i];
	}
	for(long i=0; i<msa_num; ++i)
		weight[i]/=total_weight;
	//accumulate distribution
	vector <double> accumu(msa_num,0);
	accumu[0]=weight[0];
	for(long i=1; i<msa_num; ++i)
		accumu[i]=accumu[i-1]+weight[i];



if(verbose==1)fprintf(stderr,"start sampling\n");

	//------- prepare for sample -----//
	mt19937 ws_rand;
	if(seed<0)
	{
		ws_rand.init_genrand(time(NULL));
		srand(time(NULL));
	}
	else
	{
		ws_rand.init_genrand(seed);
		srand(seed);
	}

	//------- sample K sequences -----//
	set <long> sampled_number;
	for(long i=0;i<sample_num;i++)
	{
		double randnum=ws_rand.genrand_real3();
		long retv=binarySearch_iter(accumu, 0, msa_num-1, randnum, msa_num);
		sampled_number.insert(retv);		
	}

	//------- collect sampled sequence ----//
	vector <long> sampled_index (msa_num,0);
	sampled_index[first_seq]=1;
	long sampled_num=1;
	for (set <long> :: iterator itr = sampled_number.begin(); itr != sampled_number.end(); ++itr)
	{
		long key = *itr;
		if(sampled_index[key]==0)
		{
			sampled_index[key]=1;
			sampled_num++;
		}
	}

	//------- collect unsampled sequence -----//
	long remain_num1=sample_num-sampled_num;
	long remain_num2=msa_num-sampled_num;
	long remain_num=remain_num1<remain_num2?remain_num1:remain_num2;
	if(remain_num>0)
	{
		vector <long> unsampled_index;
		for(long i=0;i<msa_num;i++)
		{
			if(sampled_index[i]==0)unsampled_index.push_back(i);
		}
		random_shuffle(unsampled_index.begin(),unsampled_index.end());
		for(long i=0;i<remain_num;i++)
		{
			long index=unsampled_index[i];
			sampled_index[index]=1;
			sampled_num++;
		}
	}
	//------- final check ----------//
	long check_num=msa_num<sample_num?msa_num:sample_num;
	if(sampled_num != check_num)
	{
		fprintf(stderr,"sampled_num %ld not equal to check_num %ld \n",
			sampled_num,check_num);
		exit(-1);
	}

	//------- output sampled K sequence -----//
	FILE *fp=fopen(axm_output.c_str(),"wb");
	for(long i=0;i<msa_num;i++)
	{
		if(sampled_index[i]==0)continue;
		if(axm_output_key==0)fprintf(fp,">%s\n",nam_list[i].c_str());
		fprintf(fp,"%s\n",fasta_list_orig[i].c_str());
	}
	fclose(fp);

	//======== exit ========//
	exit(0);
}
