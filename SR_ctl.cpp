#define PROC0_FLAG 0b00000001
#define PROC1_FLAG 0b00000010
#define PROC2_FLAG 0b00000100

#define PROC_PRINT 0b00000111	//бит показывает какой процесс отображает данные

//+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
#include <mpi.h>
//+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#include "SR_ctl.h"
//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
#define NUM_STEP_TO_STOP 1000


//////////////////////////////////////////////////////////////////////////
SR_ctl_type::SR_ctl_type()	{ }
SR_ctl_type::~SR_ctl_type()	{ }
//////////////////////////////////////////////////////////////////////////
void SR_ctl_type::Init_cfg()
{}

void SR_ctl_type::Init_local_calc()
{
	FILE *fp;
	fp=fopen("local_vars.txt", "a");	//	открытие для присоединения
	fprintf(fp,"\n    VARIABLES LIST   \n");
	
	calc_proc_cnt=Settings->find_algs(&calc_proc);
	if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);							printf("=========================================\n");	}
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (proc_rank_flag&PROC_PRINT) 	{	printf("[%d]: ",proc_rank_num);						printf("Registration of vars from .so:\n");}
	fprintf(fp," \n Registration of vars from .so:\n");
	for(int i=0;i<calc_proc_cnt;i++)
	{	
		calc_proc[i]->Reg_vars(Settings->All_local_vars);
///		calc_proc[i]->Init_consts();		
	}
	//------------------------------------------------------------------------------------
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Settings->All_local_vars->reg_out_var("ctl","local_ctl", &local_ctl_var);
	
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	Settings->All_local_vars->reg_out_var("ctl","ctl0", &out_ctl_var[0]);
	Settings->All_local_vars->reg_out_var("ctl","ctl1", &out_ctl_var[1]);	
	Settings->All_local_vars->reg_out_var("ctl","ctl2", &out_ctl_var[2]);
	Settings->All_local_vars->reg_out_var("ctl","ctl3", &out_ctl_var[3]);
	
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~	
	Settings->All_local_vars->reg_in_var("ctl","wii_ctl0", &wii_ctl_var[0]);
	Settings->All_local_vars->reg_in_var("ctl","wii_ctl1", &wii_ctl_var[1]);	
	Settings->All_local_vars->reg_in_var("ctl","wii_ctl2", &wii_ctl_var[2]);
	Settings->All_local_vars->reg_in_var("ctl","wii_ctl3", &wii_ctl_var[3]);

	//------------------------------------------------------------------------------------	
	if (proc_rank_flag&PROC_PRINT) 	{	printf("[%d]: ",proc_rank_num);							printf("=========================================\n");}
	if (proc_rank_flag&PROC_PRINT) 	{	printf("[%d]: ",proc_rank_num);							printf("Make local out vars:\n");}
	Settings->All_local_vars->make_out_vars();
	if (proc_rank_flag&PROC_PRINT) 	{	printf("[%d]: ",proc_rank_num);							printf("=========================================\n");	}
	if (proc_rank_flag&PROC_PRINT) 	{	printf("[%d]: ",proc_rank_num);							printf("Check ready state for each local calc proc:\n");	}
	for(int i=0;i<calc_proc_cnt;i++)
	{
		if (proc_rank_flag&PROC_PRINT) {printf("[%d]: ",proc_rank_num);								printf("%s:\n", calc_proc[i]->proc_name);}
		fprintf(fp,"%s: \n",calc_proc[i]->proc_name);
		calc_proc[i]->Get_ready();
	}
	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);							printf("=========================================\n");}
	//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	//+ + + + + + + + + + + + + + + + + + + + +  + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + 
	// Get the number of processes
	int world_size; 
	MPI_Comm_size(MPI_COMM_WORLD, &world_size);
	// Get the rank of the process
	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	
	//- - - - - - - - - - - -	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	p_MPI_link = new Link_MPI*[world_size-1];
	int lnk_cnt=0;
	for(int i=0;i<world_size;i++)
	{
		if(i!=world_rank)
		{
			p_MPI_link[lnk_cnt] = new Link_MPI(i);
										 printf("[%d]: ",proc_rank_num);							printf("MPI_link to %d\n",i);
										 printf("[%d]: ",proc_rank_num);							printf("MPI_link.get_node_num() = %d\n",p_MPI_link[lnk_cnt]->get_node_num());
			lnk_cnt++;
		}
	}
	//- - - - - - - - - - - -	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	
	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);							printf("process rank %d out of %d processors send outs:\n", world_rank, world_size);}
	char str_out_buf[MPI_STR_SZ];
	char str_in_buf[MPI_STR_SZ];	
	for(int i=0;i<MPI_STR_SZ;i++) {str_out_buf[i]=0; str_in_buf[i]=0;}	
	int tag = 0;				
	int dest = 0;
	int src = 0;
	if(world_rank==0){ dest = 1; src = 1;}
	
	MPI_Request resv_request;
	int resv_flag =1;	//инициализация приема
	MPI_Status resv_status;

	int loc_out_vars_sz = Settings->All_local_vars->sz_out_list();
	//выдаем другим узлам имена переменных, которые нужны но отсутствуют на данном узле 	
	int loc_in_vars_sz = Settings->All_local_vars->sz_in_list();	

	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);						printf("=!!!===============!!!==============!!!==\n");	}
	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);						printf("in out vars list %d positions\n",loc_out_vars_sz);}	
	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);						printf("in in  vars list %d positions\n",loc_in_vars_sz);}
	
	
	fprintf(fp," \n Out vars list (%d vars):\n",loc_out_vars_sz);
	for(int loc_var_idx=0;loc_var_idx<loc_out_vars_sz;loc_var_idx++)	
	{	
		SR_var_discriptor* local_out_var_ptr = Settings->All_local_vars->get_out_by_idx(loc_var_idx);
		if(local_out_var_ptr!=NULL)
		{
			if(local_out_var_ptr->use_cnt==0)	fprintf(fp,"<%s>.%s => UNLINKED \n",local_out_var_ptr->calc_proc_name,local_out_var_ptr->var_name);
			else								fprintf(fp,"<%s>.%s => linked   \n",local_out_var_ptr->calc_proc_name,local_out_var_ptr->var_name);
		}
	}
	
	fprintf(fp," \n In vars list (%d vars):\n",loc_in_vars_sz);
	for(int loc_var_idx=0;loc_var_idx<loc_in_vars_sz;loc_var_idx++)	
	{	
		SR_var_discriptor* local_in_var_ptr = Settings->All_local_vars->get_in_by_idx(loc_var_idx);
		if(local_in_var_ptr!=NULL)
		{
			if(local_in_var_ptr->use_cnt==0)	fprintf(fp,"<%s>.%s => UNLINKED \n",local_in_var_ptr->calc_proc_name,local_in_var_ptr->var_name);
			else								fprintf(fp,"<%s>.%s => linked   \n",local_in_var_ptr->calc_proc_name,local_in_var_ptr->var_name);
		}
	}	

	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);						printf("=!!!===============!!!==============!!!==\n");	}	
	int mpi_delay_timer=MPI_SR_TIMEOUT;
	int remote_vars_idx =-1;
//- - - - - - - - - - - -	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
for(int link_num=0;link_num<(world_size-1);link_num++)
{
	int dist_node_num = p_MPI_link[link_num]->get_node_num();	//номер удаленного узла
	
//	int dist_node_num = dest;	//номер удаленного узла
	int local_sz[2];	//обмен размерами буфферов первый размер - выдаваемые переменные; второй размер - требуемые переменные
	int dist_sz[2];		//обмен размерами буфферов первый размер - выдаваемые переменные; второй размер - требуемые переменные
	local_sz[0] = Settings->All_local_vars->sz_out_list();
	local_sz[1] = Settings->All_local_vars->sz_rem_list();
	
	p_MPI_link[link_num]->set_send_buff( local_sz[0] );
	p_MPI_link[link_num]->set_recv_buff( local_sz[1] );//максимально возможный размер буффера	
	
//	p_MPI_link[link_num]->set_recv_buff( (2*local_sz[1]) );//максимально возможный размер буффера	
//	p_MPI_link[link_num]->recv_buff = new float[(2*local_sz[1])];//максимально возможный размер буффера	
	MPI_Status status_sz;
	
//if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);	printf("DIST NODE[%d]",dist_node_num);}
//if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);	printf("link num[%d], DIST NODE[%d]\n", link_num,p_MPI_link[link_num]->get_node_num() );}


	MPI_Sendrecv( local_sz,2,MPI_INT,dist_node_num,tag,  dist_sz,2,MPI_INT,dist_node_num,tag,MPI_COMM_WORLD,&status_sz);
	MPI_Barrier(MPI_COMM_WORLD);
	
	if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);	printf("node[%d] declares %d outs and asks for %d ins\n", dist_node_num,dist_sz[0],dist_sz[1]);	}
	fprintf(fp," \n node[%d] declares %d outs and asks for %d ins \n \n", dist_node_num,dist_sz[0],dist_sz[1]);
	//-----------------------------------------------
	//отправка 
	for(int loc_idx=0;loc_idx<local_sz[0];loc_idx++)//отправка выдаваемых переменных
	{
		SR_var_discriptor* local_var_ptr = Settings->All_local_vars->get_out_by_idx(loc_idx);
		if(local_var_ptr!=NULL)
		{
			memset(str_out_buf, 0, MPI_STR_SZ);	
			strcpy(str_out_buf, local_var_ptr->var_name);
			MPI_Send( str_out_buf, MPI_STR_SZ, MPI_CHAR, dist_node_num, tag, MPI_COMM_WORLD);	
		//	if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);						printf("send out var: %s\n",local_var_ptr->var_name);	}
			fprintf(fp,"send out var: %s\n",local_var_ptr->var_name);
		}
	}
	fprintf(fp," \n");	
	for(int loc_idx=0;loc_idx<local_sz[1];loc_idx++)//отправка требуемых переменных
	{
		SR_var_discriptor* local_var_ptr = Settings->All_local_vars->get_rem_by_idx(loc_idx);
		if(local_var_ptr!=NULL)
		{
			memset(str_out_buf, 0, MPI_STR_SZ);	
			strcpy(str_out_buf, local_var_ptr->var_name);
			MPI_Send( str_out_buf, MPI_STR_SZ, MPI_CHAR, dist_node_num, tag, MPI_COMM_WORLD);
		//	if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);						printf("send request for unlinked in var: %s\n",local_var_ptr->var_name);	}
			fprintf(fp,"send request for unlinked in var: %s\n",local_var_ptr->var_name);
		}
	}
	fprintf(fp," \n");
	
	//-----------------------------------------------	
	//Прием
	//данные в приемном массиве (recv_buff)  хранятся в порядке, задаваемом удаленным отправителем
	int cnt_recv_buff=0;	//счетчик в буффере
	for(int dist_idx=0;dist_idx<dist_sz[0];dist_idx++)//прием удаленных выдаваемых переменных
	{
		MPI_Recv(str_in_buf, MPI_STR_SZ, MPI_CHAR, dist_node_num, tag, MPI_COMM_WORLD,  &resv_status);
	//	if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);			printf("node[%d] declares out var: %s\n",dist_node_num,str_in_buf);	}
	
		fprintf(fp,"node[%d] declares out var: %s;",dist_node_num,str_in_buf);
		for(int loc_idx=0;loc_idx<local_sz[1];loc_idx++)
		{
			SR_var_discriptor* local_var_ptr = Settings->All_local_vars->get_rem_by_idx(loc_idx);
			if(local_var_ptr!=NULL)
			{
				if( strcmp(local_var_ptr->var_name,str_in_buf)==0 ) 	//сравнение удаленного имени с требуемым
				{
					if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);	printf("node's[%d] out var <%s> will be used in func %s\n",dist_node_num,str_in_buf, local_var_ptr->calc_proc_name); }

					fprintf(fp,"	node's[%d] out var <%s> will be used in func %s",dist_node_num,str_in_buf, local_var_ptr->calc_proc_name);
//printf("before float* p_var_in_calc = *(local_var_ptr->pp_val_calc); \n");
					float* p_var_in_calc = *(local_var_ptr->pp_val_calc);			local_var_ptr->use_cnt++;
//printf("after  float* p_var_in_calc = *(local_var_ptr->pp_val_calc); \n");
					p_MPI_link[link_num]->add_recv_var(p_var_in_calc,cnt_recv_buff);//связывание принимаемых переменных с переменными приема
//printf("after  add_recv_var(p_var_in_calc,cnt_recv_buff); \n");					
					cnt_recv_buff++;
				}
			}	
		}
		fprintf(fp," \n");
	}	
	 
	p_MPI_link[link_num]->recv_sz = cnt_recv_buff;//p_MPI_link[link_num]->set_cur_recv_sz(cnt_recv_buff); //	
	if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);	printf("node[%d] will provide %d local in vars\n",dist_node_num,cnt_recv_buff); }
	fprintf(fp," \n node[%d] will provide %d local in vars\n",dist_node_num,cnt_recv_buff);

//	p_MPI_link[link_num]->shift_recv_buff = cnt_recv_buff*sizeof(float);
	
	//данные в приемном массиве (send_buff)  хранятся в порядке, задаваемом отправителем т.е. локальным процессом
	int cnt_send_buff=0;	//счетчик в буффере
//	int local_send_list_idx=0;
	for(int dist_idx=0;dist_idx<dist_sz[1];dist_idx++)//прием удаленных  требуемых переменных  
	{
		MPI_Recv(str_in_buf, MPI_STR_SZ, MPI_CHAR, dist_node_num, tag, MPI_COMM_WORLD,  &resv_status);
	//	if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);			printf("node[%d] asks for in var: %s\n",dist_node_num,str_in_buf);	}
	
		fprintf(fp,"node[%d] asks for in var: %s;",dist_node_num,str_in_buf);
		for(int loc_idx=0;loc_idx<local_sz[0];loc_idx++)
		{		
			SR_var_discriptor* local_var_ptr = Settings->All_local_vars->get_out_by_idx(loc_idx);
			if(local_var_ptr!=NULL)
			{
				if( strcmp(local_var_ptr->var_name,str_in_buf)==0 ) 	//сравнение удаленного имени с требуемым
				{			
					if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);		printf("%s.<%s> will be used in node[%d]\n",local_var_ptr->calc_proc_name,str_in_buf,dist_node_num); }

					fprintf(fp,"	%s.<%s> will be used in node[%d]",local_var_ptr->calc_proc_name,str_in_buf,dist_node_num);

					float* p_var_in_calc = *(local_var_ptr->pp_val_calc);			local_var_ptr->use_cnt++;
					//p_MPI_link[link_num]->add_send_var(p_var_in_calc,cnt_send_buff);//связывание принимаемых переменных с переменными приема
					p_MPI_link[link_num]->add_send_var(p_var_in_calc,loc_idx);//связывание принимаемых переменных с переменными приема
					cnt_send_buff++;
				}
			}
		}
		fprintf(fp," \n");
	}
	p_MPI_link[link_num]->send_sz = cnt_send_buff;//p_MPI_link[link_num]->set_cur_send_sz(cnt_send_buff);	//
	p_MPI_link[link_num]->set_local_send_buff_order();//установить порядок выдачи соответствующий локальному
	
	if (proc_rank_flag&PROC_PRINT) 	{printf("[%d]: ",proc_rank_num);	printf("%d local out vars will be got from node[%d]\n",cnt_send_buff,dist_node_num); }
	fprintf(fp," \n %d local out vars will be got from node[%d]\n",cnt_send_buff,dist_node_num);
	
	MPI_Barrier(MPI_COMM_WORLD);	
}
//- - - - - - - - - - - -	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -	
	
	MPI_Barrier(MPI_COMM_WORLD);
	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);						printf("sync by var names transfer\n");}
	//+ + + + + + + + + + + + + + + + + + + + +  + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + 
	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);						printf("=========================================\n");	}
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------
	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);						printf("=========================================\n");	}
	//-----------------------------------------------------------------------------------------------------------------------------------------------------------	
	fclose(fp);
}


void SR_ctl_type::Init()
{
		radio_ctl=false;// управление по радиоканалу отключено при старте (управлене с локального джойстика)
		//печать в консоль
		print_topic=0;
		print_block=0;
		print_alg=0;
		print_var=0;
		//		чтение настроек
		Settings= new SR_Settings();	Settings->Init();	
		if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);							printf("START CTL 12\n");}				//сигнализация начала процедуры запуска блока управления
//		initSecondaryI2CBus();
//		if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);							printf("SecondaryI2CBus On!\n");}	//инициализация второго I2c интерфейса Rasp Pi
//		Init_cfg();
//		if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);							printf("config read!\n");	}		//инициализация конфигурационной информации из файла
		print_cnt=0;							//период индикации, мс
 ///		step=0;									//период обсчета данных

		//порядок инициализации менять нельзя!!!
		if (proc_rank_flag&PROC_PRINT) 			{printf("[%d]: ",proc_rank_num);					printf("Sensors checked!\n");}	//инициализация датчиков (если есть)
											//определение типа блока
		//с этого момента известен тип блока
		Init_local_calc();											//инициализация локальных расчетов блока
		//----------------------------------------------------------------------------------------------------------------------------
		usleep(100 * 1000);											//задержка
}






 
void SR_ctl_type::Work()
{ 
		//+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
		// Get the number of processes
		int world_size;
		MPI_Comm_size(MPI_COMM_WORLD, &world_size);
		// Get the rank of the process
		int world_rank;
		MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
		if (proc_rank_flag&PROC_PRINT) 		{	printf("[%d]: ",proc_rank_num);						printf(" process rank %d out of %d processors\n", world_rank, world_size);}
		int tag = 0;				
		MPI_Request resv_request;
		int resv_flag =1;
		MPI_Status resv_status;

		if (proc_rank_flag&PROC_PRINT) 			{	printf("[%d]: ",proc_rank_num);								printf("Start! \n");	}


	//	for(step=0; true ;step+=MEMS_PERIOD,print_cnt+=MEMS_PERIOD)
	//	for(step=0; step<NUM_STEP_TO_STOP*MEMS_PERIOD ;step+=MEMS_PERIOD,print_cnt+=MEMS_PERIOD)
		for(long stp_tm=0; stp_tm<NUM_STEP_TO_STOP*MEMS_PERIOD; stp_tm+=MEMS_PERIOD,print_cnt+=MEMS_PERIOD)	
		{
			double HP_start_time = MPI_Wtime();
			//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
			//			быстрая часть
			//--------------------------------------------------------------------------------
		
			///	!!!!!Временно!!!!!
			for(int i=0;i<4;i++)	*out_ctl_var[i] =   *wii_ctl_var[i];			
			///	!!!!!Временно!!!!!
			
			
			//			/*
			//- - - - - - - - - - - -	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			//Цикл по связям
			for(int link_num=0;link_num<(world_size-1);link_num++)
			{
				int dist_node_num = p_MPI_link[link_num]->get_node_num();	//номер удаленного узла
				//+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
				p_MPI_link[link_num]->copy_send_vars();
				MPI_Send(p_MPI_link[link_num]->send_buff, p_MPI_link[link_num]->send_sz, MPI_FLOAT, dist_node_num, tag, MPI_COMM_WORLD );
				if(resv_flag==1)
				{
					p_MPI_link[link_num]->copy_recv_vars();
					MPI_Irecv( p_MPI_link[link_num]->recv_buff, p_MPI_link[link_num]->recv_sz, MPI_FLOAT, dist_node_num, tag, MPI_COMM_WORLD,  &resv_request);
				}
				MPI_Test(&resv_request, &resv_flag, &resv_status);
				//+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
			}
			//- - - - - - - - - - - -	- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
			//	*/
			
			
			//--------------------------------------------------------------------------------
			for(int i=0;i<calc_proc_cnt;i++)
			{
				if( MEMS_PERIOD <= (calc_proc[i]->calc_period) && (calc_proc[i]->calc_period) < PRINT_PERIOD)
				{		
					if(print_topic==TOPIC_ALG_MSGS && print_block==proc_rank_num && print_alg==i)
					{
						*local_ctl_var= -1;
						printf("[%d]:alg[%d]<%s>msg: ",proc_rank_num,i,calc_proc[i]->proc_name);
						calc_proc[i]->calc();
						printf("\n");
					}
					else
					{
						*local_ctl_var= 0;
						calc_proc[i]->calc();
					}
				}
			}		
			//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++				
			//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++					
			if(print_cnt>PRINT_PERIOD)
			{	
				if(*out_ctl_var[0] == CMD_STOP_PROG)
				printf("CMD_STOP_PROG\n");

				//printf("[%d]: ",proc_rank_num);	printf("Main calc: \n");
				for(int i=0;i<calc_proc_cnt;i++)
				{
					if( PRINT_PERIOD <= calc_proc[i]->calc_period )
					{
						if(print_topic==TOPIC_ALG_MSGS && print_block==proc_rank_num && print_alg==i)
						{
							*local_ctl_var= -1;
							printf("[%d]:ALG[%d]<%s>MSG: ",proc_rank_num,i,calc_proc[i]->proc_name);
							calc_proc[i]->calc();
							printf("\n");
						}
						else
						{
							*local_ctl_var= 0;
							calc_proc[i]->calc();
						}
					}
				}
				print_cnt=0;//сброс таймера
				//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				if(/*print_topic==TOPIC_ALG_MSGS && */print_block==proc_rank_num && print_alg==-1)
				{				
					printf ("alg ctl[-1] data ");
					if(radio_ctl)	printf ("(radio): ");
					else			printf ("(wii): ");

					printf ("\n");
				}
				//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
				//++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
	
				//-----------------------------------------------------------------------------------------------------
				//-----------------------------------------------------------------------------------------------------
				if(*out_ctl_var[0] == CMD_STOP_PROG)		{		break;				}
			//	if(*out_ctl_var[0] != CMD_STOP_PROG) *out_ctl_var[0] = 0;	//все команды выдаются однократно, а команда стоп - висит
				//-----------------------------------------------------------------------------------------------------
				//-----------------------------------------------------------------------------------------------------
			}
			
			print_func();
			
			step_time = (MPI_Wtime()-HP_start_time)*1000;//время цикла в мс
			
			int idle_time = MEMS_PERIOD - (int)step_time;//
			if(idle_time<0)
			{
				printf("[%d]: ",proc_rank_num); printf ("Worning: step time overlimits MEMS_PERIOD!\n");
				idle_time=0;
			}	
			
			usleep(idle_time);
        }
		//-----------------------------------------------------------------------------------------------------
		//-----------------------------------------------------------------------------------------------------
		if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);				printf("final\n");	}		
		//-----------------------------------------------------------------------------------------------------
		//-----------------------------------------------------------------------------------------------------
}
void SR_ctl_type::print_func()//печать в консоль
{
	//-------------------------------------------------------------------------------------------	
	float ctl0 = *out_ctl_var[0];
	float ctl1 = *out_ctl_var[1];
	float ctl2 = *out_ctl_var[2];
	float ctl3 = *out_ctl_var[3];
	
	if(ctl0==CMD_NO_PRINT)				
	{ print_topic=0; 												printf("[%d]:CMD_NO_PRINT (step time= %.5f s)\n",proc_rank_num,step_time);		}		
	
	if(ctl0==CMD_PRINT_ALG_VAR)				
	{	print_topic=TOPIC_ALG_VARS; print_var=0; print_alg=0;
																	printf("[%d]:CMD_PRINT_ALG_VAR \n",proc_rank_num);	}
	if(ctl0==CMD_PRINT_ALG_MSG)				
	{	print_topic=TOPIC_ALG_MSGS;									printf("[%d]:CMD_PRINT_ALG_MSG\n",proc_rank_num);	}
	

	
//	if(ctl0==CMD_PRINT_BLOCK )		{/*print_block=0;*/					printf("[%d]:print_block=0\n",proc_rank_num);	}
//	if(ctl0==CMD_PRINT_ALG 	)		{/*print_alg=0;*/					printf("[%d]:print_alg=0\n",proc_rank_num);		}
//	if(ctl0==CMD_PRINT_VAR 	)		{/*print_var=0;*/					printf("[%d]:print_var=0\n",proc_rank_num);		}

	if(false && ctl0==CMD_INC_PRINT_BLOCK )	{print_block++;					printf("[%d]:print_block =%d\n",proc_rank_num,print_block);	}
	if(false && ctl0==CMD_DEC_PRINT_BLOCK )	{print_block--;					printf("[%d]:print_block =%d\n",proc_rank_num,print_block);	}

	if(ctl0==CMD_INC_PRINT_ALG   )	{print_alg++;					printf("[%d]:print_alg =%d\n"  ,proc_rank_num,print_alg);	}
	if(ctl0==CMD_DEC_PRINT_ALG   )	{print_alg--;					printf("[%d]:print_alg =%d\n"  ,proc_rank_num,print_alg);	}

	if(false && ctl0==CMD_INC_PRINT_VAR   )	{print_var++;					printf("[%d]:print_var =%d\n"  ,proc_rank_num,print_var);	}
	if(false && ctl0==CMD_DEC_PRINT_VAR   )	{print_var--;					printf("[%d]:print_var =%d\n"  ,proc_rank_num,print_var);	}
	//-------------------------------------------------------------------------------------------
	if(print_topic ==0) return;	//не надо печатать
	if(print_block!=proc_rank_num)	return;	//не надо печатать этому блоку	
	
	if(print_topic == TOPIC_ALG_VARS)
	{
		printf("[%d]: ",proc_rank_num);
		
		if(0<=print_alg && print_alg<calc_proc_cnt)
		{
			printf("alg[%d]<%s>var:",print_alg,calc_proc[print_alg]->proc_name);
			
			int var = abs(print_var)-1; if(var<0)var=0;
			if(print_var>0)		printf("out[%d]<%s> = %.3f",var,calc_proc[print_alg]->Get_out_name(var),calc_proc[print_alg]->Get_out_val(var));
			if(print_var<0)		printf(" in[%d]<%s> = %.3f",var,calc_proc[print_alg]->Get_in_name(var), calc_proc[print_alg]->Get_in_val(var));	
		
		}
		else
		{
			printf("alg[%d]<no alg>...",print_alg);
		}
		printf("\n");		
	}
	//		int calc_proc_num_out_var=calc_proc[calc_proc_idx]->Get_out_val_num();
	//		int calc_proc_num_in_var =calc_proc[calc_proc_idx]->Get_in_val_num();	
}

int main(int argc, char *argv[])
{
	printf("MPI start\n");
	// Initialize the MPI environment
	MPI_Init(NULL, NULL);	//MPI_Init(&argc, &argv);       //

	int world_rank;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	
	proc_rank_num = world_rank;
	if(world_rank==0)	proc_rank_flag = PROC0_FLAG;
	if(world_rank==1)	proc_rank_flag = PROC1_FLAG;
	if(world_rank==2)	proc_rank_flag = PROC2_FLAG;
	//+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +
	SR_ctl_type*	SR_ctl = new SR_ctl_type();
	SR_ctl->Init();
	SR_ctl->Work();
	//+ + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + + +	
	// Finalize the MPI environment.
	MPI_Finalize();
	if (proc_rank_flag&PROC_PRINT) 		{printf("[%d]: ",proc_rank_num);					printf("MPI fin\n");}	
	
	return 0;	
}
