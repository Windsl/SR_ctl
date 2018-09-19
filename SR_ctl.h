#include "config_SR.h"

#include <dlfcn.h>
#include <link.h>
//-------------------------------
#include <dirent.h>
//-------------------------------
#define N 10

#define CMD_ARRAY_SZ 8
#define GRIP_POS_ARRAY_SZ 16

#define CMD_NUM_PAR 4


//+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
#define MPI_AR_SZ 10
#define MPI_STR_SZ 128
#define MPI_SR_DELAY 50
#define MPI_SR_TIMEOUT 1000 
//+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +

 
char proc_rank_flag;
char proc_rank_num;


class Motor_drive
{
public:
	const char* name;		//имя привода
	bool is;	
private:
	int fd;				//файл-дескритор устройства	
	float max_speed;	//придел по скорости
public:
	Motor_drive();
	~Motor_drive();
	int Init(const char *dev_str, const char *dev_name,float speed_limit);
	int SetSpeed(float speed);
	void Stop();
	void Close();
};

//класс
class SR_ctl_type
{
//переменные
private:
		Link_MPI** p_MPI_link;

		double step_time;//время цикла

		bool radio_ctl;//признак управления по радиоканалу
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		float* local_ctl_var;
		
		//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
		float*  wii_ctl_var[4];
		float* out_ctl_var[4];

		int print_topic;
		int print_block;
		int print_alg;
		int print_var;
		

		SR_Settings* Settings;
		
		SR_calc_proc** calc_proc;
		int calc_proc_cnt;

		int stop_timer;						//таймер останова программы управления

		int print_cnt;			//шаг обсчета медленных функций и выдачи на печать (мс)
	 
		//---------------------------------------------------------------------------------------------------------
		//Wii
		int joyX;		int joyY;		
        int accelX;		int accelY;		int accelZ;
        int buttonC;        int buttonZ;
		int joyX0;		int joyY0;
        int accelX0;	int accelY0;	int accelZ0;
	    float inv_joyX0;
	    float inv_joyY0;	
		//---------------------------------------------------------------------------------------------------------	
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		int G_idx, A_idx, M_idx;	
		int Gx, Gy, Gz;			
		int Ax, Ay, Az;	
		int Mx, My, Mz;
		int Vx, Vy, Vz;
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
		Motor_drive* motor_board_Left ;	
		Motor_drive* motor_board_Right;	
		Motor_drive* motor_board_Pitch;	
		Motor_drive* motor_kite_Left;	
		Motor_drive* motor_kite_Right;	
		//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++	
//методы
public:
	SR_ctl_type();
	~SR_ctl_type();
	void Init();
	void Work();	
private:
	void print_func();
	
	void Init_local_calc();
	void Init_cfg();		
	void Init_GPS();	
	void Init_motors();
};