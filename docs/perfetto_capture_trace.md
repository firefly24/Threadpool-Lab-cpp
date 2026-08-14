# Steps to Capture a Perfetto trace 

### Setup backgroud services:
	
	cd ~/perfetto/out/linux

	./traced --background
	sudo ./traced_probes --background
	
### Verify : 

perfetto.traced_probes should appear under connected producers

	sudo ./perfetto --query
	
	
## Create scheduler trace configuration: 

	cd ~/perfetto/out/linux

	cat > sched.cfg <<'EOF'
	buffers: {
	  size_kb: 32768
	  fill_policy: RING_BUFFER
	}

	data_sources: {
	  config {
		name: "linux.ftrace"
		ftrace_config {
		  ftrace_events: "sched/sched_switch"
		  ftrace_events: "sched/sched_waking"
		  ftrace_events: "sched/sched_wakeup"
		}
	  }
	}
	
	data_sources: {
  		config {
    		name: "track_event"
  		}
  	}

	duration_ms: 10000
	EOF

### Capture the trace : 
	
	sudo ./perfetto --txt \
    -c sched.cfg \
    -o ~/threadpool-sched.perfetto-trace
    


