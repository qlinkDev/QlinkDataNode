
#include<stdio.h> 
#include<time.h> 
#include<math.h> 
#include<stdlib.h> 
#include <string.h>
#include "common_qlinkdatanode.h"
  
  

static unsigned long long g = 2147483647;

static unsigned long long n = 2147483645;


/**********************************************************************
* SYMBOL	: runFermatPow(void)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/12/29/
**********************************************************************/
// module multiplication 
static unsigned long long runFermatPow(unsigned long long p, unsigned int r, unsigned long long g){  

   unsigned long long result = 1;  
	
        while(r) {
            if(r % 2) {
                result = result * p % g;
                r -= 1;
            } else {
                p = p * p % g;
                r /= 2;
            }
        }

	 result = result %n;
	 LOG("Debug:  result = %lld.\n", result);
	 
        return result;

   } 

/**********************************************************************
* SYMBOL	: Random_num(void)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/12/29/
**********************************************************************/
//generate random number
static unsigned int Random_num() 
{ 
	unsigned int num = 0; 

	srand(time(NULL)); 
	num = rand() % (16384) + 16384; 

	LOG("Debug:  num = %d.\n", num);
	
	return num; 
} 


/**********************************************************************
* SYMBOL	: GenerateKeyUeVlaue(void)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: save the rand_ue.
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/12/29/
**********************************************************************/
unsigned int GenerateKeyUeVlaue(char *p_key){

	unsigned int rand_ue;
 	unsigned long long k_ue,key_temp, key;
 	int i=0;
	char p_temp_key[8];

	//p_temp_key = p_key;

	memset(p_temp_key,0,sizeof(p_temp_key));
	rand_ue = Random_num();
	LOG("Debug:  rand_ue = %d.\n", rand_ue);
	/*k_ue should be sent to server to calculate the key on server side. */
 	k_ue = runFermatPow(g,rand_ue,n);
	LOG("Debug:  k_ue = %lld.\n", k_ue);

	/*add the key to array k[8] for encryption */ 
	key_temp = k_ue;
	LOG("Debug:  key_temp = %lld.\n", key_temp);
	i=7;
	while((key_temp!=0)&&(i>=0))
	{
	p_temp_key[i]=p_temp_key[i]^(key_temp&0xFF);
	//*(p_temp_key+i) = (*(p_temp_key+i))^(key_temp&0xFF);
	key_temp=(key_temp>>8);
	i--;
	LOG("Debug:  key_temp = %lld.\n", key_temp);
	LOG("Debug:  p_temp_key[i] = 0x%02x.\n", p_temp_key[i]);
	}

	memcpy(p_key,p_temp_key,sizeof(p_temp_key));

	LOG("Debug:  rand_ue = %d.\n", rand_ue);
	return rand_ue;

}

/**********************************************************************
* SYMBOL	: Hex_to_long
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: NULL
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2015/12/28
**********************************************************************/
unsigned long long Hex_to_long( char *pabybuff ,unsigned int i_num)
{
	int i;
	unsigned int value;
	char *pabdata;

	if ((pabybuff == NULL) || (i_num > 8))
		return -1;

	pabdata = pabybuff;

	value = 0;

	for(i=0; i < i_num; i++)
	{
		value <<= 8;
		value |= ( *(pabdata+i) & 0x000000ff);
	}

	return value;
}


/**********************************************************************
* SYMBOL	: unsigned long long GetKeyVlaueFromServerProcess(char *p_key, unsigned int un_rand,char *p_outkey)
* FUNCTION	: 
* PARAMETER	: 
*			: 
* RETURN	: 
*			: 
* NOTE		:
* AUTHOR	: JACKLI
* DATE		: 2016/12/29/
**********************************************************************/
unsigned long long GetKeyVlaueFromServerProcess(char *p_key, unsigned int un_rand,char *p_outkey){

	
 	unsigned long long key_temp,key_server;
 	int i=0;
	char p_temp_key[8];

	//p_temp_key = p_outkey;

	memset(p_temp_key, 0 , sizeof(p_temp_key));
	key_temp = Hex_to_long(p_key,8);
	LOG("Debug:  key_temp = %lld.\n", key_temp);

	LOG("Debug:  un_rand = %d.\n", un_rand);
	
	/*k_ue should be sent to server to calculate the key on server side. */
 	key_server = runFermatPow(key_temp,un_rand,n);
	LOG("Debug:  key_server = %lld.\n", key_server);
	
	/*add the key_server to array k[8] for encryption */ 
	i=7;
	while((key_server!=0)&&(i>=0))
	{
	p_temp_key[i]=p_temp_key[i]^(key_server&0xFF);
	//*(p_temp_key+i) = (*(p_temp_key+i))^(key_server&0xFF);
	key_server = (key_server>>8);
	i--;
	
	}

	memcpy(p_outkey, p_temp_key , sizeof(p_temp_key));

	/*for(i = 0; i < 8; i++){

		LOG("Debug:  p_outkey[i] = 0x%02x.\n", p_outkey[i]);

	}*/

	
	
	LOG("Debug:  key_server = %lld.\n", key_server);
	return key_server;

}
	

#if 0
	
/*an example to caculate key */	
 int main()  
{     
	
	 	unsigned int rand_ue;
 		unsigned long long k_ue,key_temp, key;
 		int i=0;  
 		
 	 	 	printf("g=%lld",g);
 		printf("\n");
 	
 	 	 	printf("n=%lld",n);
 		printf("\n");
 		
 		
 	rand_ue = Random_num();
 	printf("rand_ue=%d",rand_ue);
 	printf("\n");
 	
 	
 	/*k_ue should be sent to server to calculate the key on server side. */
 	k_ue = runFermatPow(g,rand_ue,n);
 	 	printf("k_ue=%lld",k_ue);
 		printf("\n");
 	

/*calculate key which is used on encryption  */
 	
 	key = runFermatPow(k_server,rand_ue,n);
 	 printf("key=%lld",key);
 		printf("\n");
 	
 	

 		
/*add the key to array k[8] for encryption */ 
	
	key_temp=key;
	i=7;
	while((k_temp!=0)&&(i>=0))
	{
	k[i]=k[i]^(k_temp&0xFF);
	k_temp=(k_temp>>8);
	i--;
	}
}

#endif

