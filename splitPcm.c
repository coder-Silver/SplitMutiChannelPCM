/**
*  split multi-channel pcm data to single channel pcm data
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "stdint.h"

#define	SEEK_SET 0 //set file offset to offset
#define SEEK_CUR 1 //set file offset to current plus offset
#define SEEK_END 2 //set file offset to EOF plus offset

#define SAMPLE_RATE 16000
#define SAMPLE_BIT 16


#define ID_RIFF 0x46464952
#define ID_WAVE 0x45564157
#define ID_FMT  0x20746d66
#define ID_DATA 0x61746164
#define FORMAT_PCM 1
//#define DEBUG_TEST 1


#define PERIOD_SIZE  1024 

#define SINGLE_FILE "Channel_"
#define AUDIO_FORMAT "wav"
#define NAME_LENGTH  20


struct wav_header
{	
	uint32_t riff_id;
    uint32_t riff_sz;
    uint32_t riff_fmt;
    uint32_t fmt_id;
    uint32_t fmt_sz;
    uint16_t audio_format;
    uint16_t num_channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
    uint32_t data_id;
    uint32_t data_sz;

};
typedef struct wav_header wav_header_t ;

int get_wav_channel(FILE * file) ; 
int split_multiChannels_to_single_wav(char* wav_file) ;
int split_multi_channels_pcm_to_single_channel(wav_header_t header , char* multi_channel_buffer,uint32_t size ,char ** sub_channel_buffer,uint32_t single_channel_size);
int split_multiChannels_to_single_wav(char* wav_file);  


void printHeader(wav_header_t header)
{
	printf("header size = %d\n",sizeof(header)) ; 
	printf("riff_id = %d\n riff_sz = %d\n riff_fmt = %d\n fmt_id = %d\n fmt_sz = %d \n audio_format = %d\n num_channels = %d\n sample_rate =%d \n byte_rate = %d\n block_align = %d\n bits_per_sample = %d\n data_id = %d\n data_sz = %d\n\n ",
		    header.riff_id,header.riff_sz,header.riff_fmt,header.fmt_id,header.fmt_sz,header.audio_format,
		    header.num_channels,header.sample_rate,header.byte_rate,header.block_align,header.bits_per_sample,header.data_id,header.data_sz );

}

/**
 * 传入wav 文件,获取通道数
 */
int get_wav_channel(FILE * file)
{
	if(!file) return -1 ; 
	int header_size = sizeof(struct wav_header);
	wav_header_t * header = (wav_header_t*)malloc(header_size) ; 

	int size = fread(header,1,header_size,file) ; 
	if(size != header_size)
	{
		printf("read file header failed \n");
		return -1 ; 
	}
	int num_channels = header->num_channels ; 
	free(header) ; 
    return num_channels ; 
}
/** 
 * wav_file：wav音频文件流
 * wav_header:存放获取的wav文件头
 */
int  get_wav_header(FILE * wav_file,wav_header_t * wav_header)
{
	if(!wav_header) return -1 ;
	uint16_t header_size = sizeof(struct wav_header);
	uint32_t size =  fread(wav_header,1,header_size,wav_file) ;
	if(size != header_size)
	{
		printf("get header read file failed\n");
		return -1 ; 
	}
	return 0 ; 

}

/**
*  header:多通道wav头
*  multi_channel_buffer：多通道pcm源数据
*  size：多通道数据buffer长度
*  sub_channel_buffer：存放分解后多个单通道数据
*  single_channel_size：分解后单通道数据长度
*/
int split_multi_channels_pcm_to_single_channel(wav_header_t header , char* multi_channel_buffer,uint32_t size ,char ** sub_channel_buffer,uint32_t single_channel_size) 
{
	uint32_t block_align = header.block_align ;
	uint32_t num_channels = header.num_channels ; 
	uint16_t sample_bit_width = header.bits_per_sample >> 3 ; 
	uint32_t single_block_nums = size /block_align ;
	uint16_t single_block_align =   block_align/num_channels ; 
	#ifdef DEBUG_TEST
	printf("single_block_nums = %d\n",single_block_nums );
	#endif 
	uint32_t k = 0 ;
	uint32_t a0 = 0 ; 
	for(int i = 0 ; i < header.num_channels ; i++)
	{
		uint32_t n = 0 ;
		for(int j = 1 ; j <= single_block_nums ;j++)
		{
			k = a0+single_block_align*num_channels* (j-1); 
			n = single_block_align*(j-1) ;
			#ifdef DEBUG_TEST
			printf("n = %d   k = %d \n", n,k);
			#endif 
			sub_channel_buffer[i][n]=multi_channel_buffer[k];
			sub_channel_buffer[i][n+1] = multi_channel_buffer[k+1] ; 
		}
		a0 += 2 ; 
	}
	return 0 ; 
}

/**
*  将多通道pcm数据分解成对应单通道数据
*/
int split_multiChannels_to_single_wav(char* wav_file)
{
	FILE * file = fopen(wav_file,"rb");
	if(!file)
	{
		printf("open file failed %s\n",wav_file);
		return -1 ; 
	}
	struct wav_header header ; 
	get_wav_header(file,&header) ; 
	uint32_t size = PERIOD_SIZE * header.num_channels * (header.bits_per_sample >> 3) ; 
	uint32_t single_channel_size = PERIOD_SIZE *(header.bits_per_sample >> 3);

    
	FILE * channel_files[header.num_channels] ; 
	for(int f_index = 0 ; f_index < header.num_channels ; f_index ++)
	{
		char single_file_name [NAME_LENGTH] ; 
		uint32_t len = sprintf(single_file_name,"Channel_%d.wav",f_index+1);
		channel_files[f_index] = fopen(single_file_name,"wb");
		if(!channel_files[f_index])
		{
			printf("unable create file %d\n",channel_files[f_index] );
		}
		printf("file name %s \n",single_file_name);
	}
	
	printf("num_channels = %d  buffer_size=%d   block_align = %d,bits_per_sample = %d sample_rate = %d  data_sz = %d\n"
		,header.num_channels, size ,header.block_align,header.bits_per_sample ,header.sample_rate,header.data_sz);
	printHeader(header) ; 
	
	char* multi_buffer = (char*)malloc(size);
	char * sub_buffers[header.num_channels]  ;

	for(int i = 0 ; i < header.num_channels ; i++)
	{
		sub_buffers[i] = (char*)malloc(single_channel_size);
		if(!sub_buffers[i])
		{
			printf("malloc mem failed\n");
			return -1 ; 
		}

		fseek(channel_files[i],sizeof(struct wav_header),SEEK_SET);//预留文件头空间
	}
	


	struct wav_header header_one ; 
	memcpy(&header_one,&header,sizeof(struct wav_header)) ;

	uint32_t read_size =fread(multi_buffer,1,size,file) ; 
	uint32_t write_size = 0 ; 
	printf("read_size =%d \n",read_size);
	clock_t start , end ; 
	while(read_size)
	{	
		split_multi_channels_pcm_to_single_channel(header,multi_buffer,size,sub_buffers,single_channel_size) ;
		for( int index = 0 ; index < header.num_channels ; index++)
		{
			write_size=fwrite(sub_buffers[index],1,single_channel_size,channel_files[index]);
			if(write_size!= single_channel_size)
			{
				printf("write single buffer failed  %d\n",index );
			}	
		}
		read_size =fread(multi_buffer,1,size,file) ;

	}
	header_one.num_channels = 1 ; 
	header_one.block_align = header_one.bits_per_sample >> 3 ; 
	header_one.byte_rate = (header_one.bits_per_sample >> 3)*header_one.sample_rate ; 
	header_one.data_sz = header.data_sz/header.num_channels ; 
	header_one.riff_sz = header_one.data_sz+sizeof(header_one)-8 ; 

	for(int ind = 0 ; ind < header.num_channels ; ind++)
	{
		fseek(channel_files[ind],0,SEEK_SET);
		fwrite(&header_one,sizeof(header_one),1,channel_files[ind]);
	}

    printf("num_channels = %d   block_align = %d,bits_per_sample = %d sample_rate = %d  data_sz = %d\n"
		,header_one.num_channels  ,header_one.block_align,header_one.bits_per_sample ,header_one.sample_rate,header_one.data_sz);
    printHeader(header_one) ; 
    

   
    printf("extract finished !\n");

	free(multi_buffer) ; 
	for(int j = 0 ; j < header.num_channels;j++)
	{
		free(sub_buffers[j]);
		fclose(channel_files[j]) ;
	}
	free(sub_buffers) ; 
	 
	fclose(file) ; 

	return 0 ; 
}


void test_clock()
{
	double start = clock() ; 
	for (int i = 0 ; i < 1000000 ; i++)
	{
		;
	}
	double end = clock() ; 
	printf("start = %lf end = %lf   compute time = %lf \n", start  ,end, (end-start)/CLK_TCK *1000   );
}

int main(int argsc, char ** argsv)
{
	if(argsc < 1)
	{
		printf("too few params\n" );
		return -1 ; 
	}
	
	printf("file name =  %s \n ",argsv[1]);//argsv[1]

	split_multiChannels_to_single_wav(argsv[1]) ; 
	return 0 ; 
}




