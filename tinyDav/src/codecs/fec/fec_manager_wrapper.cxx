#ifdef ANDROID
#include "tinydav/codecs/fec/fec_manager.h"
#else
#include "fec_manager.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif




struct fecManager
{	
	fec_encode_manager_t *fec_encode_manager;
	fec_decode_manager_t *fec_decode_manager;
};
 

struct fecManager *getFecInstance(){
	char rs_par_str[rs_str_len]="20:10";
    g_fec_par.rs_from_str(rs_par_str);
	struct fecManager * ret = (struct fecManager *)malloc(sizeof(struct fecManager));
	if (ret) {
		ret->fec_encode_manager = new fec_encode_manager_t();
		ret->fec_decode_manager = new fec_decode_manager_t();
	}
	return ret;
}

void releaseFecManager(struct fecManager **ppInstance){
	if (ppInstance == NULL || *ppInstance == NULL){
		return;
	}
	struct fecManager * fec = *ppInstance;
	if (fec->fec_encode_manager) {
		delete fec->fec_encode_manager;
		fec->fec_encode_manager = NULL;
	}
	if (fec->fec_decode_manager) {
		delete fec->fec_decode_manager;
		fec->fec_decode_manager = NULL;
	}
	free(*ppInstance);
    *ppInstance = 0;
}


int initFecParameter(struct fecManager *pInstance, int k, int n, int mtu){
	if (pInstance == NULL || pInstance->fec_encode_manager == NULL){
		return -1;
	}
	fec_parameter_t &fec_par=pInstance->fec_encode_manager->get_fec_par();
	fec_par.mtu= mtu;//g_fec_par.mtu;
	fec_par.queue_len= g_fec_par.queue_len;
	fec_par.timeout= g_fec_par.timeout;
	fec_par.mode=1;
	char rsFormat[125] = {'\0'};
	sprintf(rsFormat, "%d:%d", k, n);
	fec_par.rs_from_str(rsFormat);


	return 0;
}

int inputEncode(struct fecManager *pInstance,char *s,int len/*,int &is_first_packet*/){
	if (pInstance == NULL || pInstance->fec_encode_manager == NULL){
		return -1;
	}

	return pInstance->fec_encode_manager->input(s, len);
}

int outputEncode(struct fecManager *pInstance,int *n, char *** s_arr,int **len){
	if (pInstance == NULL || pInstance->fec_encode_manager == NULL){
		return -1;
	}

	return pInstance->fec_encode_manager->output(*n, *s_arr, *len);
}


int inputDecode(struct fecManager *pInstance,char *s,int len/*,int &is_first_packet*/){
	if (pInstance == NULL || pInstance->fec_decode_manager == NULL){
		return -1;
	}

	return pInstance->fec_decode_manager->input(s, len);
}

int outputDecode(struct fecManager *pInstance,int *n,char *** s_arr,int **len){
	if (pInstance == NULL || pInstance->fec_decode_manager == NULL){
		return -1;
	}

	return pInstance->fec_decode_manager->output(*n, *s_arr, *len);
}

#ifdef __cplusplus
};
#endif
