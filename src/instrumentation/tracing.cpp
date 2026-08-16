#include "threadpool/instrumentation/tracing.hpp"

#ifdef THREADPOOL_ENABLE_TRACING

PERFETTO_TRACK_EVENT_STATIC_STORAGE();

#endif


void tracing::threadpool::initialize()
{

#ifdef THREADPOOL_ENABLE_TRACING
	perfetto::TracingInitArgs args;
	args.backends |= perfetto::kSystemBackend;
	
	perfetto::Tracing::Initialize(args);
	perfetto::TrackEvent::Register();
	
#endif
	
	return;
}
