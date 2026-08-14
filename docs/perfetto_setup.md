# Perfetto Setup & Trace Recording — Jetson Orin Nano (Linux ARM64 platform)

## 1. Clone Perfetto

bash
cd ~
git clone https://github.com/google/perfetto.git
cd perfetto


## 2. Install python venv support

Perfetto's dependency installer requires python virtual environments

Check python version: 
	python3 --version
	
For Python 3.10:
	
	sudo apt update
	sudo apt install python3.10-venv
	
## 3. Install Perfetto dependencies

Make sure to run as NORMAL user, not as sudo: 
	cd ~/perfetto
	rm -rf .venv
	tools/install-build-deps
	
## 4. Install system gn + ninja

On Jetson (linux ARM64), perfetto's normal build wrappers can expect x86-64 toolchains. Install native system tools instead: 
	
	sudo apt install gn ninja-build
	gn --version
	ninja --version
	
## 5. Generate ARM64 build: 
	
Use system compiler instad of perfetto's incompatible clang toolchain
	
	cd ~/perfetto
	gn gen out/linux --args='
	is_debug=false
	is_clang=false
	is_system_compiler=true
	'

## 6. Build Perfetto: 

	ninja -C out/linux \
    perfetto \
    traced \
    traced_probes \
    trace_processor_shell
	
Successful build will producee:
	out/linux/perfetto
	out/linux/traced
	out/linux/traced_probes
	out/linux/trace_processor_shell
	
Verify: 
	out/linux/perfetto --version
	
	
## If want to build perfetto SDK: 

	cd ~/perfetto

	tools/gen_amalgamated \
	  --output sdk/perfetto \
	  --sdk cpp \
	  --gn_args 'is_clang=false is_system_compiler=true enable_perfetto_re2=false enable_perfetto_zstd=false enable_perfetto_zlib=false enable_perfetto_stderr_crash_dump=false'
		
	
	
