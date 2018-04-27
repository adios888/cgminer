/* 
 * File:   arrays.h：动态数组函数头文件
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

#ifndef _ARRAYS_H
#define	_ARRAYS_H
#ifdef	__cplusplus
extern "C" {
#endif

#define DefaultArraySize 10/**默认没有设置初始化大小时的大小*/
#define ArrayPerAddSize 1   /**每次不够用时增加多少项，大于1可能会引起size失真（略大于实际值）*/
#define GetArray(type,arr,index) ((type *)ArrayGet(arr,index))
    //这个地方是用来申明一个动态数组比如：struct mystruct *my=Darray(struct mystruct,10);就申请了一个10个数组的结构体
 #define Darray(type,length) (type *)New(sizeof(type)*length)
    typedef struct _arrays Array;

    struct _arrays {
        int size; /*总大小*/
        int index; /**当前赋值最大索引项*/
        void * item[1][2];
    };
/*
 *功能：实现一个Array结构体的初始化，能够包含DefaultArraySize大小个项目，每一个数组项需要重新内存分配
 *     不接受非内存动态分配对象（栈上的数据，比如临时申请的字符串），否则会引起错误（函数在返回后可能就不存在了）
 *参数：void
 *返回：Array结构体
 */
extern Array * ArrayNew();
/*
 *功能：实现一个Array结构体的初始化，容量为给定的size
 *参数：size:容器大小
 *返回：Array数组
 */
extern Array * NewArray(int size);
/*
 *功能：返回给定数组指定索引处的值，索引以0开头
 *参数：arr:给定的结构体，index:索引
 *返回：void指针，如果为空或者给定Index大于数组的最大长度就返回NULL
 */
extern void * ArrayGet(Array * arr,int index);
/*
 *功能：设置给定数组的给定索引处的值
 *声明：添加堆的数据到指定的Index处。函数有可能改变原有内存位置，用新的内存来代替，所以参数需要数组的指针地址
 *     如果Index大于容器体积，那么他会增加递归调用，只要满足能够让index包含，如果给定Index处已经有了堆分配的数据
 *     那么先释放数据，然后才设定新数据
 *参数： arr:给定的数组指针地址 index：要设置项的索引  value：设定的值
 *返回：void
 */
extern void ArraySet(Array ** arr,int index,void * value);
/*
 *功能：设置给定数组的给定索引处的值
 *声明：添加堆的数据到指定的Index处。函数有可能改变原有内存位置，用新的内存来代替，所以参数需要数组的指针地址
 *     如果Index大于容器体积，那么他会增加递归调用，只要满足能够让index包含，如果给定Index处已经有了堆分配的数据
 *     那么先释放数据，然后才设定新数据
 *参数： arr:给定的数组指针地址 index：要设置项的索引  value：设定的值
 *返回：void
 */
extern void SetArray(Array ** arr,int index,void * value);
/*
 *功能：向指定数组最后增加一个值，若数组容量不够就会重新申请更大的容积数组，然后拷贝数据再加入新的
 *声明：value必须为栈中的数据，Array函数不提供统一释放栈项内容，函数有可能改变原有
 *    内存位置，用新的内存来代替，所以参数需要数组的指针地址
 *参数：arr:给定的数组指针地址 value：设定的值
 *返回：void
 */
extern void ArrayAdd(Array ** arr,void * value);
/*
 *功能：向指定数组最后增加一个值，若数组容量不够就会重新申请更大的容积数组，然后拷贝数据再加入新的
 *声明：alue必须是堆中申请的内存，不然函数有可能改变原有内存位置，用新的内存来代替，所以参数需要数组的指针地址
 *参数：arr:给定的数组指针地址 value：设定的值
 *返回：void
 */
extern void AddArray(Array ** arr, void * value);
/*
 *功能：增加指定数组的容两量
 *声明：有可能引起新内存申请，内存拷贝，从而改变指针具体指向
 *参数：arr:要操作的数组指针地址 size:要增加的数量
 *返回：数组指针
 */
extern Array * ArrayAddSize(Array ** arr,int size);
/*
 *功能：返回指定数组的容积大小
 *参数：arr:指定数组
 *返回：容积大小
 */
extern int ArraySize(Array * arr);
/*
 *功能：释放指定数组内存
 *参数：arr:指定数组
 *返回：void
 */
extern void ArrayFree(Array * arr);
/*
 *公能：数组功能测试
 *参数：void
 *返回：void
 */
extern void ArrayTest();


#ifdef	__cplusplus
}
#endif

#endif	/* _ARRAYS_H */

