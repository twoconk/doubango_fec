
#ifndef __FEC_MANAGER_WRAPPER__
#define __FEC_MANAGER_WRAPPER__


struct fecManager;
#ifdef __cplusplus
extern "C" {
#endif

struct fecManager *getFecInstance();
int initFecParameter(struct fecManager *pInstance, int k, int n, int mtu);
int outputDecode(struct fecManager *pInstance,int *n,char *** s_arr,int **len);
int inputDecode(struct fecManager *pInstance,char *s,int len);
int outputEncode(struct fecManager *pInstance,int *n, char *** s_arr,int **len);
int inputEncode(struct fecManager *pInstance,char *s,int len);

void releaseFecManager(struct fecManager **ppInstance); 
#ifdef __cplusplus
};
#endif


#endif//__FEC_MANAGER_WRAPPER__