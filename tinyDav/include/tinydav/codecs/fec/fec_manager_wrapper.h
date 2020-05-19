
#ifndef __FEC_MANAGER_WRAPPER__
#define __FEC_MANAGER_WRAPPER__


struct fecManager;
#ifdef __cplusplus
extern "C" {
#endif

/*为C程序封装的初始化构造接口*/
struct fecManager *getFecInstance();

/*为C程序封装的释放接口*/
void releaseFecManager(struct fecManager **ppInstance);

/*k为源包个数，n为冗余包个数，mtu建议小余1400，默认值为1350*/
int initFecParameter(struct fecManager *pInstance, int k, int n, int mtu);

/*输入源数据包，进行FEC编码运算*/
int inputEncode(struct fecManager *pInstance, char *s, int len);

/*输出FEC编码包，可能是多个数据包，需要再调用完inputEncode后，马上调用，否则下次的inputEncode会报错*/
int outputEncode(struct fecManager *pInstance, int *n, char *** s_arr, int **len);


/*输入FEC编码包，进行FEC解码 */
int inputDecode(struct fecManager *pInstance, char *s, int len);

/*输出源包，可能得到多个源包*/
int outputDecode(struct fecManager *pInstance, int *n, char *** s_arr, int **len);

#ifdef __cplusplus
};
#endif


#endif//__FEC_MANAGER_WRAPPER__
