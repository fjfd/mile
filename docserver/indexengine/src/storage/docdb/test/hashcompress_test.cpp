
#include "../../../common/def.h"
#include <gtest/gtest.h>
#include "../../../common/mem.h"
#include "../hash_memcompress.h"
#include "../doclist.h"

void get_low_data(struct low_data_struct* data,mem_pool* mem_pool)
{
	data->len = 5;
	data->data = mem_pool_malloc(mem_pool,5);
	data->type = HI_TYPE_STRING;
	data->field_name = (char*)mem_pool_malloc(mem_pool,20);
	memset(data->field_name,0,20);
	strcpy(data->field_name,"hi_type_string");
	memset(data->data,0,5);
	strcpy((char*)data->data,"ali");

	return;
}

void verify_muti_hashvalue(struct hash_index_manager* hash_index,MEM_POOL* mem_pool)
{
	//查询性能测试
	struct rowid_list* rlist_a;
	struct rowid_list* rlist_b;
	uint64_t time_a = 0;
	uint64_t time_b = 0;
	uint64_t now;
	uint32_t i;

	struct hash_compress_manager* hash_compress = hash_compress_load(hash_index,10,mem_pool);

	struct low_data_struct* data = (struct low_data_struct*)mem_pool_malloc(mem_pool,sizeof(struct low_data_struct));
	get_low_data(data,mem_pool);
	 
	for(i=0; i<hash_index->limit;i++)
	{	sprintf((char*)data->data,"%d",i);
	
		now = get_time_usec();
		rlist_a = hash_index_query(hash_index,data,mem_pool);
		time_a += get_time_usec() - now;
	
		now = get_time_usec();
		rlist_b = hash_compress_query(hash_compress,data,mem_pool);
		time_b += get_time_usec()-now;
	
		ASSERT_EQ(rowid_list_equals(rlist_a,rlist_b),1);
	}
	
	
	 printf("hash_index %llu hash_compress %llu\n",time_a,time_b);
	 hash_compress_release(hash_compress);
}


//多个hash值测试
void test_multi_hashvalue(uint32_t limit)
{
   MEM_POOL* mem_pool = mem_pool_init(MB_SIZE);
   int32_t ret;
   struct hash_index_config config;
   config.row_limit = limit;
   strcpy(config.work_space,"/tmp/hash_compress_test");
   system("rm -rf /tmp/hash_compress_test");
   mkdirs(config.work_space);
   init_profile(1000,mem_pool);
   
   struct hash_index_manager* hash_index = hash_index_init(&config,mem_pool);


   uint32_t i;
   struct low_data_struct* data = (struct low_data_struct*)mem_pool_malloc(mem_pool,sizeof(struct low_data_struct));
   get_low_data(data,mem_pool);
   
   for(i=0; i<config.row_limit;i++)
   {   sprintf((char*)data->data,"%d",i);
	   ret = hash_index_insert(hash_index,data,i);
	   ASSERT_EQ(0,ret);
   }

   verify_muti_hashvalue(hash_index,mem_pool);
   hash_index_release(hash_index);
}

//单个hash值测试
void test_single_hashvalue(uint32_t limit)
{
   MEM_POOL* mem_pool = mem_pool_init(MB_SIZE);
   struct hash_index_config config;
   config.row_limit = limit;
   strcpy(config.work_space,"/tmp/hash_compress_test");
   system("rm -rf /tmp/hash_compress_test");
   
   mkdirs(config.work_space);
   init_profile(1000,mem_pool);
   
   struct hash_index_manager* hash_index = hash_index_init(&config,mem_pool);


   uint32_t i;
   struct low_data_struct* data = (struct low_data_struct*)mem_pool_malloc(mem_pool,sizeof(struct low_data_struct));
   get_low_data(data,mem_pool);
   
   for(i=0; i<config.row_limit;i++)
   {   
	   hash_index_insert(hash_index,data,i);
   }

	struct hash_compress_manager* hash_compress = hash_compress_load(hash_index,10,mem_pool);
	
   	struct rowid_list* rlist_a;
	struct rowid_list* rlist_b;
	
	rlist_a = hash_index_query(hash_index,data,mem_pool);
	rlist_b = hash_compress_query(hash_compress,data,mem_pool);

	ASSERT_EQ(rowid_list_equals(rlist_a,rlist_b),1);
	
   hash_index_release(hash_index);
   hash_compress_release(hash_compress);
}


//测试两个hash value的值，包含超过100个大批量的docid
void test_tow_hashvalue(uint32_t limit)
{
   MEM_POOL* mem_pool = mem_pool_init(MB_SIZE);
   struct hash_index_config config;
   config.row_limit = limit;
   strcpy(config.work_space,"/tmp/hash_compress_test");
   system("rm -rf /tmp/hash_compress_test");
   mkdirs(config.work_space);
   init_profile(1000,mem_pool);
   
   struct hash_index_manager* hash_index = hash_index_init(&config,mem_pool);


   uint32_t i;
   struct low_data_struct* data = (struct low_data_struct*)mem_pool_malloc(mem_pool,sizeof(struct low_data_struct));
   get_low_data(data,mem_pool);
   
   for(i=0; i<config.row_limit/2;i++)
   {   
   	   strcpy((char*)data->data,"1");
	   hash_index_insert(hash_index,data,i);
   }

   for(i=config.row_limit/2; i<config.row_limit;i++)
   {   
   	   strcpy((char*)data->data,"2");
	   hash_index_insert(hash_index,data,i);
   }

   	struct hash_compress_manager* hash_compress = hash_compress_load(hash_index,10,mem_pool);
	
   	struct rowid_list* rlist_a;
	struct rowid_list* rlist_b;

    get_low_data(data,mem_pool);

	strcpy((char*)data->data,"1");
	rlist_a = hash_index_query(hash_index,data,mem_pool);
	rlist_b = hash_compress_query(hash_compress,data,mem_pool);

	ASSERT_EQ(rowid_list_equals(rlist_a,rlist_b),1);

	strcpy((char*)data->data,"2");
	rlist_a = hash_index_query(hash_index,data,mem_pool);
	rlist_b = hash_compress_query(hash_compress,data,mem_pool);

	ASSERT_EQ(rowid_list_equals(rlist_a,rlist_b),1);

    hash_index_release(hash_index);
    hash_compress_release(hash_compress);
}



TEST(HASHCOMPRESS_TEST, HandleNoneZeroInput) {
	test_multi_hashvalue(10);
	test_multi_hashvalue(100);
	test_multi_hashvalue(1000);
	test_multi_hashvalue(10000);

	test_single_hashvalue(10);
	test_single_hashvalue(100);
	test_single_hashvalue(1000);
	test_single_hashvalue(10000);

	test_tow_hashvalue(10);
	test_tow_hashvalue(100);
	test_tow_hashvalue(1000);
	test_tow_hashvalue(10000);
}



int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


