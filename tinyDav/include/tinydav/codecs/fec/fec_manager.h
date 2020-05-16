/*
 * fec_manager.h
 *
 *  Created on: Sep 27, 2017
 *      Author: root
 */

#ifndef FEC_MANAGER_H_
#define FEC_MANAGER_H_

#ifdef ANDROID
#include "tsk_params.h"
#include "tsk_memory.h"
#include "tsk_debug.h"
#include "tsk_time.h"
#include "tinydav_config.h"
#include "tinymedia/tmedia_codec.h"
#else
 #define TSK_DEBUG_INFO printf
 #define TSK_DEBUG_WARN printf
 #define TSK_DEBUG_ERROR printf
#endif

#include "tinydav/codecs/fec/rs.h"

#include <inttypes.h> 
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <time.h>
//#include <netinet/in.h>
//#include <net/if.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h> 
#include <errno.h> //For errno - the error number
//#include <unordered_map>
//#include <unordered_set>
#include <map>
#include <list>
#include <string>
#include <vector>
using  namespace std;


typedef unsigned long long u64_t;   //this works on most platform,avoid using the PRId64
typedef long long i64_t;

typedef unsigned int u32_t;
typedef int i32_t;

typedef unsigned short u16_t;
typedef short i16_t;

/*common.h*/


const int max_addr_len=100;
const int max_data_len=3600;
const int buf_len=max_data_len+200;

const int default_mtu=1250;

//const u32_t timer_interval=400;
////const u32_t conv_timeout=180000;
//const u32_t conv_timeout=40000;//for test
const u32_t conv_timeout=180000;
const int max_conv_num=10000;
const int max_conn_num=200;

/*
const u32_t max_handshake_conn_num=10000;
const u32_t max_ready_conn_num=1000;
//const u32_t anti_replay_window_size=1000;


const u32_t client_handshake_timeout=5000;
const u32_t client_retry_interval=1000;


const u32_t server_handshake_timeout=10000;// this should be much longer than clients. client retry initially ,server retry passtively*/

const int conv_clear_ratio=30;  //conv grabage collecter check 1/30 of all conv one time
const int conn_clear_ratio=50;
const int conv_clear_min=1;
const int conn_clear_min=1;

const u32_t conv_clear_interval=1000;
const u32_t conn_clear_interval=1000;


const i32_t max_fail_time=0;//disable

const u32_t heartbeat_interval=1000;

const u32_t timer_interval=400;//this should be smaller than heartbeat_interval and retry interval;
/*common.h end.*/


const int max_blob_packet_num=30000;//how many packet can be contain in a blob_t ,can be set very large
const u32_t anti_replay_buff_size=30000;//can be set very large

const int max_fec_packet_num=255;// this is the limitation of the rs lib
extern u32_t fec_buff_num;

const int rs_str_len=max_fec_packet_num*10+100;
extern int header_overhead;
extern int debug_fec_enc;
extern int debug_fec_dec;


static vector<string> string_to_vec(const char * s,const char * sp) {
	  vector<string> res;
	  string str=s;
	  char *p = strtok ((char *)str.c_str(),sp);
	  while (p != NULL)
	  {
		 res.push_back(p);
	    //printf ("%s\n",p);
	    p = strtok(NULL, sp);
	  }

	 /* for(int i=0;i<(int)res.size();i++)
	  {
		  printf("<<%s>>\n",res[i].c_str());
	  }*/
	  return res;
}
 
static u64_t get_current_time()//ms
{
	timespec tmp_time;
	clock_gettime(CLOCK_MONOTONIC, &tmp_time);
	return ((u64_t)tmp_time.tv_sec)*1000llu+((u64_t)tmp_time.tv_nsec)/(1000*1000llu);
}

static u64_t get_current_time_us()
{
	timespec tmp_time;
	clock_gettime(CLOCK_MONOTONIC, &tmp_time);
	return (uint64_t(tmp_time.tv_sec))*1000llu*1000llu+ (uint64_t(tmp_time.tv_nsec))/1000llu;
}


static u64_t pack_u64(u32_t a,u32_t b)
{
	u64_t ret=a;
	ret<<=32u;
	ret+=b;
	return ret;
}
static u32_t get_u64_h(u64_t a)
{
	return a>>32u;
}
static u32_t get_u64_l(u64_t a)
{
	return (a<<32u)>>32u;
}

static void write_u16(char * p,u16_t w)
{
	*(unsigned char*)(p + 1) = (w & 0xff);
	*(unsigned char*)(p + 0) = (w >> 8);
}
static u16_t read_u16(char * p)
{
	u16_t res;
	res = *(const unsigned char*)(p + 0);
	res = *(const unsigned char*)(p + 1) + (res << 8);
	return res;
}

static void write_u32(char * p,u32_t l)
{
	*(unsigned char*)(p + 3) = (unsigned char)((l >>  0) & 0xff);
	*(unsigned char*)(p + 2) = (unsigned char)((l >>  8) & 0xff);
	*(unsigned char*)(p + 1) = (unsigned char)((l >> 16) & 0xff);
	*(unsigned char*)(p + 0) = (unsigned char)((l >> 24) & 0xff);
}
static u32_t read_u32(char * p)
{
	u32_t res;
	res = *(const unsigned char*)(p + 0);
	res = *(const unsigned char*)(p + 1) + (res << 8);
	res = *(const unsigned char*)(p + 2) + (res << 8);
	res = *(const unsigned char*)(p + 3) + (res << 8);
	return res;
}

static void write_u64(char * s,u64_t a)
{
	assert(0==1);
}
static u64_t read_u64(char * s)
{
	assert(0==1);
	return 0;
}
 
static int round_up_div(int a,int b)
{
	return (a+b-1)/b;
}

struct fec_parameter_t
{
public:
	int version;
	int mtu;
	int queue_len;
	int timeout;
	int mode;
	int rs_cnt;

public:
    fec_parameter_t(){
	 version=0;
	 mtu=default_mtu;
	 queue_len=200;
	 timeout=8*1000;
	 mode=0;
	 rs_cnt=0;
    }

	struct rs_parameter_t //parameters for reed solomon
	{
		unsigned char x;//AKA fec_data_num  (x should be same as <index of rs_par>+1 at the moment)
		unsigned char y;//fec_redundant_num
	}rs_par[max_fec_packet_num+10];

	int rs_from_str(char * s)//todo inefficient
	{
		vector<string> str_vec=string_to_vec(s,",");
		if(str_vec.size()<1)
		{
			TSK_DEBUG_WARN("failed to parse [%s]\n",s);
			return -1;
		}
		vector<rs_parameter_t> par_vec;
		for(int i=0;i<(int)str_vec.size();i++)
		{
			rs_parameter_t tmp_par;
			string &tmp_str=str_vec[i];
			int x,y;
			if(sscanf((char *)tmp_str.c_str(),"%d:%d",&x,&y)!=2)
			{
				TSK_DEBUG_WARN("failed to parse [%s]\n",tmp_str.c_str());
				return -1;
			}
			if(x<1||y<0||x+y>max_fec_packet_num)
			{
				TSK_DEBUG_WARN("invaild value x=%d y=%d, x should >=1, y should >=0, x +y should <%d\n",x,y,max_fec_packet_num);
				return -1;
			}
			tmp_par.x=x;
			tmp_par.y=y;
			par_vec.push_back(tmp_par);
		}
		assert(par_vec.size()==str_vec.size());

		int found_problem=0;
		for(int i=1;i<(int)par_vec.size();i++)
		{
			if(par_vec[i].x<=par_vec[i-1].x)
			{
				TSK_DEBUG_WARN("error in [%s], x in x:y should be in ascend order\n",s);
				return -1;
			}
			int now_x=par_vec[i].x;
			int now_y=par_vec[i].y;
			int pre_x=par_vec[i-1].x;
			int pre_y=par_vec[i-1].y;

			double now_ratio=double(par_vec[i].y)/par_vec[i].x;
			double pre_ratio=double(par_vec[i-1].y)/par_vec[i-1].x;

			if(pre_ratio+0.0001<now_ratio)
			{
				if(found_problem==0)
				{
					TSK_DEBUG_WARN("possible problems: %d/%d<%d/%d",pre_y,pre_x,now_y,now_x);
					found_problem=1;
				}
				else
				{
					TSK_DEBUG_WARN(", %d/%d<%d/%d",pre_y,pre_x,now_y,now_x);
				}
			}
		}
		if(found_problem)
		{
			TSK_DEBUG_WARN(" in %s\n",s);
		}

		{ //special treatment for first parameter
			int x=par_vec[0].x;
			int y=par_vec[0].y;
			for(int i=1;i<=x;i++)
			{
				rs_par[i-1].x=i;
				rs_par[i-1].y=y;
			}
		}

		for(int i=1;i<(int)par_vec.size();i++)
		{
			int now_x=par_vec[i].x;
			int now_y=par_vec[i].y;
			int pre_x=par_vec[i-1].x;
			int pre_y=par_vec[i-1].y;
			rs_par[now_x-1].x=now_x;
			rs_par[now_x-1].y=now_y;

			double now_ratio=double(par_vec[i].y)/par_vec[i].x;
			double pre_ratio=double(par_vec[i-1].y)/par_vec[i-1].x;

			//double k= double(now_y-pre_y)/double(now_x-pre_x);
			for(int j=pre_x+1;j<=now_x-1;j++)
			{
				int in_x=j;

			////////	int in_y= double(pre_y) + double(in_x-pre_x)*k+ 0.9999;// round to upper

			double distance=now_x-pre_x;
			///////	double in_ratio=pre_ratio*(1.0-(in_x-pre_x)/distance)   +   now_ratio *(1.0- (now_x-in_x)/distance);
			//////	int in_y= in_x*in_ratio + 0.9999;
				int in_y= pre_y +(now_y-pre_y) *(in_x-pre_x)/distance +0.9999;

				if(in_x+in_y>max_fec_packet_num)
				{
					in_y=max_fec_packet_num-in_x;
					assert(in_y>=0&&in_y<=max_fec_packet_num);
				}

				rs_par[in_x-1].x=in_x;
				rs_par[in_x-1].y=in_y;
			}
		}
		rs_cnt=par_vec[par_vec.size()-1].x;

		return 0;
	}

	char *rs_to_str()//todo inefficient
	{
		static char res[rs_str_len];
		string tmp_string;
		char tmp_buf[100];
		assert(rs_cnt>=1);
		for(int i=0;i<rs_cnt;i++)
		{
			sprintf(tmp_buf,"%d:%d",int(rs_par[i].x),int(rs_par[i].y));
			if(i!=0)
				tmp_string+=",";
			tmp_string+=tmp_buf;
		}
		strcpy(res,tmp_string.c_str());
		return res;
	}

	rs_parameter_t get_tail()
	{
		assert(rs_cnt>=1);
		return rs_par[rs_cnt-1];
	}


	int clone(fec_parameter_t & other)
	{
		version=other.version;
		mtu=other.mtu;
		queue_len=other.queue_len;
		timeout=other.timeout;
		mode=other.mode;

		assert(other.rs_cnt>=1);
		rs_cnt=other.rs_cnt;
		memcpy(rs_par,other.rs_par,sizeof(rs_parameter_t)*rs_cnt);

		return 0;
	}


};

extern fec_parameter_t g_fec_par;
//extern int dynamic_update_fec;

const int anti_replay_timeout=120*1000;// 120s


struct my_itimerspec {
	struct timespec it_interval;  /* Timer interval */
	struct timespec it_value;     /* Initial expiration */
};

typedef u64_t my_time_t;

struct anti_replay_t
{

	struct info_t
	{
		my_time_t my_time;
		int index;
	};

	u64_t replay_buffer[anti_replay_buff_size];
	map<u32_t,info_t> mp;
	int index;
	anti_replay_t()
	{
		clear();
	}
	int clear()
	{
		memset(replay_buffer,-1,sizeof(replay_buffer));
		mp.clear();
		//mp.rehash(anti_replay_buff_size*3);
		index=0;
		return 0;
	}
	void set_invaild(u32_t seq)
	{

		if(is_vaild(seq)==0)
		{
			TSK_DEBUG_INFO("seq %u exist\n",seq);
			//assert(mp.find(seq)!=mp.end());
			//mp[seq].my_time=get_current_time_rough();
			return;
		}
		if(replay_buffer[index]!=u64_t(i64_t(-1)))
		{
			assert(mp.find(replay_buffer[index])!=mp.end());
			mp.erase(replay_buffer[index]);
		}
		replay_buffer[index]=seq;
		assert(mp.find(seq)==mp.end());
		mp[seq].my_time=get_current_time();
		mp[seq].index=index;
		index++;
		if(index==int(anti_replay_buff_size)) index=0;
	}
	int is_vaild(u32_t seq)
	{
		if(mp.find(seq)==mp.end()) return 1;
		
		if(get_current_time()-mp[seq].my_time>anti_replay_timeout)
		{
			replay_buffer[mp[seq].index]=u64_t(i64_t(-1));
			mp.erase(seq);
			return 1;
		}

		return 0;
	}
};

struct blob_encode_t
{
	char input_buf[(max_fec_packet_num+5)*buf_len];
	int current_len;
	int counter;

	char *output_buf[max_fec_packet_num+100];

	blob_encode_t();

    int clear();

    int get_num();
    int get_shard_len(int n);
    int get_shard_len(int n,int next_packet_len);

	int input(char *s,int len);  //len=use len=0 for second and following packet
	int output(int n,char ** &s_arr,int & len);
};

struct blob_decode_t
{
	char input_buf[(max_fec_packet_num+5)*buf_len];
	int current_len;
	int last_len;
	int counter;

	char *output_buf[max_blob_packet_num+100];
	int output_len[max_blob_packet_num+100];

	blob_decode_t();
	int clear();
	int input(char *input,int len);
	int output(int &n,char ** &output,int *&len_arr);
};

//class fec_encode_manager_t:not_copy_able_t
class fec_encode_manager_t
{

private:
	u32_t seq;

	//int fec_mode;
	//int fec_data_num,fec_redundant_num;
	//int fec_mtu;
	//int fec_queue_len;
	//int fec_timeout;
	fec_parameter_t fec_par;


	my_time_t first_packet_time;
	my_time_t first_packet_time_for_output;


	blob_encode_t blob_encode;
	char input_buf[max_fec_packet_num+5][buf_len];
	int input_len[max_fec_packet_num+100];

	char *output_buf[max_fec_packet_num+100];
	int output_len[max_fec_packet_num+100];

	int counter;
	//int timer_fd;
	//u64_t timer_fd64;

	int ready_for_output;
	u32_t output_n;

	int append(char *s,int len);
/*
	ev_timer timer;
	struct ev_loop *loop=0;
	void (*cb) (struct ev_loop *loop, struct ev_timer *watcher, int revents)=0;
*/
public:
	fec_encode_manager_t();
	~fec_encode_manager_t();

	fec_parameter_t & get_fec_par()
	{
		return fec_par;
	}
	void set_data(void * data)
	{
		//timer.data=data;
	}

/*

	void set_loop_and_cb(struct ev_loop *loop,void (*cb) (struct ev_loop *loop, struct ev_timer *watcher, int revents))
	{
		this->loop=loop;
		this->cb=cb;
		ev_init(&timer,cb);
	}

*/
	int clear_data()
	{
		counter=0;
		blob_encode.clear();
		ready_for_output=0;

		seq= 0;//(u32_t)get_fake_random_number(); //TODO temp solution for a bug.

/*

		if(loop)
		{
			ev_timer_stop(loop,&timer);
		}
*/
		return 0;
	}
	int clear_all()
	{

		//itimerspec zero_its;
		//memset(&zero_its, 0, sizeof(zero_its));
		//timerfd_settime(timer_fd, TFD_TIMER_ABSTIME, &zero_its, 0);

/*
		if(loop)
		{
			ev_timer_stop(loop,&timer);
			loop=0;
			cb=0;
		}
*/
		clear_data();

		return 0;
	}

	my_time_t get_first_packet_time()
	{
		return first_packet_time_for_output;
	}

	int get_pending_time()
	{
		return fec_par.timeout;
	}

	int get_type()
	{
		return fec_par.mode;
	}
	//u64_t get_timer_fd64();
	int reset_fec_parameter(int data_num,int redundant_num,int mtu,int pending_num,int pending_time,int type);
	int input(char *s,int len/*,int &is_first_packet*/);
	int output(int &n,char ** &s_arr,int *&len);
};
struct fec_data_t
{
	int used;
	u32_t seq;
	int type;
	int data_num;
	int redundant_num;
	int idx;
	char buf[buf_len];
	int len;
};
struct fec_group_t
{
	int type;
	int data_num;
	int redundant_num;
	int len;
	int fec_done;
	//int data_counter=0;
	map<int,int>  group_mp;

	fec_group_t(){

	 type=-1;
	 data_num=-1;
	 redundant_num=-1;
	 len=-1;
	 fec_done=0;
	}
};
//class fec_decode_manager_t:not_copy_able_t
class fec_decode_manager_t
{
	anti_replay_t anti_replay;
	fec_data_t *fec_data;
	map<u32_t, fec_group_t> mp;
	blob_decode_t blob_decode;

	int index;

	int output_n;
	char ** output_s_arr;
	int * output_len_arr;
	int ready_for_output;

	char *output_s_arr_buf[max_fec_packet_num+100];//only for type=1,for type=0 the buf inside blot_t is used
	int output_len_arr_buf[max_fec_packet_num+100];//same

public:
	fec_decode_manager_t()
	{
		fec_data=new fec_data_t[fec_buff_num+5];
		assert(fec_data!=0);
		clear();
	}
	/*
	fec_decode_manager_t(const fec_decode_manager_t &b)
	{
		assert(0==1);//not allowed to copy
	}*/
	~fec_decode_manager_t()
	{
		TSK_DEBUG_INFO("fec_decode_manager destroyed\n");
		if(fec_data!=0)
		{
			TSK_DEBUG_INFO("fec_data freed\n");
			delete fec_data;
		}
	}
	int clear()
	{
		anti_replay.clear();
		mp.clear();
		//mp.rehash(fec_buff_num*3);

		for(int i=0;i<(int)fec_buff_num;i++)
			fec_data[i].used=0;
		ready_for_output=0;
		index=0;

		return 0;
	}

	//int re_init();
	int input(char *s,int len);
	int output(int &n,char ** &s_arr,int* &len_arr);
};

#endif /* FEC_MANAGER_H_ */
