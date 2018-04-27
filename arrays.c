/* 
 * File:   arrays.c：动态数组函数实现
 * 声明：此函数功能不具备线程安全性，所以在使用过程中不建议用于公共变量，建议在函数内部使用
 *      所有以Array开头的函数说明参数内存为栈，所有以Array结尾的函数说明参数内存为堆
 *      所有传入数组的堆内存，一旦调用FreeAray或者于内存操作相关的函数就会将其全部释放，不需要逐个释放
 * Author: netpet
 * Flower net server
 * 本程序是为一体化web server产品专用设计，具有部分代码为具体产品优化而不代表普遍通用性的特性
 * 程序在linux 2.46下调试通过，编辑工具netbeans 6.1 for c
 * 联系方式：Email:netpetboy@163.com QQ:51977431
 * Created on 2008年5月29日, 上午10:44
 */
#include "arrays.h"
#include <stdio.h>
#include <stdlib.h>
#define New malloc
#define Free free
/*
 *功能：实现一个Array结构体的初始化，能够包含DefaultArraySize大小个项目，每一个数组项需要重新内存分配
 *     不接受非内存动态分配对象（栈上的数据，比如临时申请的字符串），否则会引起错误（函数在返回后可能就不存在了）
 *参数：void
 *返回：Array结构体
 */
Array * ArrayNew()
{
	Array * arr=(Array *)New(2*DefaultArraySize*(sizeof(void *))+sizeof(Array));
	arr->size=DefaultArraySize;
	arr->index=0;
	int i;
	for(i=0;i<arr->size;i++)
        {
		arr->item[i][0]=NULL;
                //第二个参数用来记录该项是栈还是堆的内存，便于释放时使用
                arr->item[i][1]=NULL;
        }
	return arr;
}
/*
 *功能：实现一个Array结构体的初始化，容量为给定的size
 *参数：size:容器大小
 *返回：Array数组
 */
Array * NewArray(int size)
{
	Array * arr=(Array *)New(2*size*(sizeof(void *))+sizeof(Array));
	arr->size=size;
	arr->index=0;
	int i;
	for(i=0;i<arr->size;i++)
        {
		arr->item[i][0]=NULL;
                //第二个参数用来记录该项是栈还是堆的内存，便于释放时使用
                arr->item[i][1]=NULL;
        }
	return arr;
}
/*
 *功能：返回给定数组指定索引处的值，索引以0开头
 *参数：arr:给定的结构体，index:索引
 *返回：void指针，如果为空或者给定Index大于数组的最大长度就返回NULL
 */
void * ArrayGet(Array * arr,int index)
{
	if(index<arr->size)
	  return arr->item[index][0];
	else
		return NULL;
}
/*
 *功能：设置给定数组的给定索引处的值
 *声明：添加栈的数据到Array中。函数有可能改变原有内存位置，用新的内存来代替，所以参数需要数组的指针地址
 *     如果Index大于容器体积，那么他会增加递归调用，只要满足能够让index包含
 *参数： arr:给定的数组指针地址 index：要设置项的索引  value：设定的值
 *返回：void
 */
void ArraySet(Array ** arr,int index,void * value)
{

	if(index>(*arr)->index)
		(*arr)->index=index;

	if(index<(*arr)->size)
        {
             if( ((*arr)->item[index][0])!=NULL&&((*arr)->item[index][1])!=NULL)
            {
                Free((*arr)->item[index][0]);
                (*arr)->item[index][1]=NULL;
            }
	   (*arr)->item[index][0]=value;
        }
	else
	{
		ArrayAddSize(arr,ArrayPerAddSize);
		ArraySet(arr,index,value);
	}
}
/*
 *功能：设置给定数组的给定索引处的值
 *声明：添加堆的数据到指定的Index处。函数有可能改变原有内存位置，用新的内存来代替，所以参数需要数组的指针地址
 *     如果Index大于容器体积，那么他会增加递归调用，只要满足能够让index包含，如果给定Index处已经有了堆分配的数据
 *     那么先释放数据，然后才设定新数据
 *参数： arr:给定的数组指针地址 index：要设置项的索引  value：设定的值
 *返回：void
 */
void SetArray(Array ** arr,int index,void * value)
{

	if(index>(*arr)->index)
		(*arr)->index=index;

	if(index<(*arr)->size)
        {
            //如果这个位置已经有了数据，并且数据为堆的数据，那么释放他
            if( ((*arr)->item[index][0])!=NULL&&((*arr)->item[index][1])!=NULL)
            {
                Free((*arr)->item[index][0]);
                (*arr)->item[index][1]=NULL;
                //这里本来有数据，结果被重置了,并且没有发出警报
            }
	   (*arr)->item[index][0]=value;
           (*arr)->item[index][1]=value;
        }
	else
	{
		ArrayAddSize(arr,ArrayPerAddSize);
		SetArray(arr,index,value);
	}

	
}
/*
 *功能：向指定数组最后增加一个值，若数组容量不够就会重新申请更大的容积数组，然后拷贝数据再加入新的
 *声明：value必须为栈中的数据，Array函数不提供统一释放栈项内容，函数有可能改变原有
 *    内存位置，用新的内存来代替，所以参数需要数组的指针地址
 *参数：arr:给定的数组指针地址 value：设定的值
 *返回：void
 */
void ArrayAdd(Array ** arr,void * value)
{
   if ((*arr)->index >= (*arr)->size)
        ArrayAddSize(arr, ArrayPerAddSize);
    
    (*arr)->item[(*arr)->index][0] = value;
    (*arr)->index++;
}

/*
 *功能：向指定数组最后增加一个值，若数组容量不够就会重新申请更大的容积数组，然后拷贝数据再加入新的
 *声明：alue必须是堆中申请的内存，不然函数有可能改变原有内存位置，用新的内存来代替，所以参数需要数组的指针地址
 *参数：arr:给定的数组指针地址 value：设定的值
 *返回：void
 */
void AddArray(Array ** arr, void * value) {

    if ((*arr)->index >= (*arr)->size)
        ArrayAddSize(arr, ArrayPerAddSize);
    (*arr)->item[(*arr)->index][0] = value;
     //数据为堆的数据，需要显示释放，用这个指针来标识
        (*arr)->item[(*arr)->index][1] = value;
    (*arr)->index++;

}
/*
 *功能：增加指定数组的容两量
 *声明：有可能引起新内存申请，内存拷贝，从而改变指针具体指向
 *参数：arr:要操作的数组指针地址 size:要增加的数量
 *返回：数组指针
 */
Array * ArrayAddSize(Array ** arr,int size)
{
	Array * arrr=(Array *)New(((*arr)->size+size)*2*(sizeof(void *))+sizeof(Array));
	arrr->size=(*arr)->size+size;
	int i;
	for(i=0;i<arrr->size;i++)
	{
		if(i<(*arr)->size)
                {
			arrr->item[i][0]=(*arr)->item[i][0];
                        arrr->item[i][1]=(*arr)->item[i][1];
                }
		else
                {
		   arrr->item[i][0]=NULL;
                    arrr->item[i][1]=NULL;
                }
	}
        (arrr->index)=((*arr)->index);
	Free(*arr);
	*arr=arrr;
	return arrr;
}
/*
 *功能：返回指定数组的容积大小
 *参数：arr:指定数组
 *返回：容积大小
 */
int ArraySize(Array * arr)
{
	return arr->size;
}
/*
 *功能：释放指定数组内存
 *参数：arr:指定数组
 *返回：void
 */
void ArrayFree(Array * arr)
{
	int i;
	for(i=0;i<arr->size;i++)
	{
		if((arr->item[i][0]!=NULL)&&(arr->item[i][1]!=NULL))
                {
			Free(arr->item[i][0]);
                }
	}
	Free(arr);
}
/*
 *公能：数组功能测试
 *参数：void
 *返回：void
 */
void ArrayTest()
{
    Array *arr=ArrayNew();
    int i=0;
    //测试从堆分配内存放入数组中
    char *tmp=(char *)New(1200);
    AddArray(&arr,tmp);
    //测试从同一个位置多次次设置
    char *tmp1=(char *)New(1200);
    ArraySet(&arr,0,"ergreg");
     char *tmp2=(char *)New(1200);
    SetArray(&arr,0,tmp2);
    //测试多次Add和Get
    for(i=3;i<100;i++)
    {
         ArrayAdd(&arr,"ok ");
         printf("第%i项结果为%s,Index为%d：size为：%d\n",i,ArrayGet(arr,i),arr->index,arr->size);
    }
   ArrayFree(arr);
}
