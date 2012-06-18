#include <stdlib.h>
#include <querysort/querysort.h>

#include "vrt.h"
#include "bin/varnishd/cache.h"

#include "vcc_if.h"

int
init_function(struct vmod_priv *priv, const struct VCL_conf *conf)
{
	return 0;
}

const char *
vmod_sort(struct sess *sp, const char *uri)
{
	if (uri == NULL) {
		return NULL;
	}
	
	struct ws *ws = sp->wrk->ws;
	char *sorted_uri = WS_Alloc(ws, strlen(uri) + 1);
	
	WS_Assert(ws);
	
	if (sorted_uri == NULL) {
		return NULL;
	}
	
	if (qs_sort(uri, sorted_uri) != QS_OK) {
		return NULL;
	}
	
	return sorted_uri;
}
