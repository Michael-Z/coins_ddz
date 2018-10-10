SVR_DIR = 	proto 			\
			./svr/common				\
			./svr/room/src 			\
			./svr/routing/src 		\
			./svr/dbproxy/src			\
			./svr/connect/src			\
			./svr/gm/src			\
			./svr/hall/src			\
			./svr/log/src			\
			./svr/user/src 			\
			./svr/teabar/src 			\
			./svr/ddz_es/src			\
			./svr/ddz_jd/src			\
			./svr/ddz_mc/src			\
			./svr/ddz_tj/src			\
			./svr/ddz_yc/src			\
			./svr/gzp_tc/src			\
			./svr/mj_bd/src			\
			./svr/mj_es/src			\
			./svr/mj_minxian/src			\
			./svr/pdk_classic/src			\
			./svr/pdk_xf/src			\
			./svr/pdk_hcng/src			\
			./svr/sdr_3jing/src			\
			./svr/sdr_5jing/src			\
			./svr/sdr_bd/src			\
			./svr/sdr_bd_new/src			\
			./svr/sdr_bh/src	\
			./svr/sdr_ch/src	\
			./svr/sdr_dyfj/src	\
			./svr/sdr_es/src			\
			./svr/sdr_es_match/src 	\
			./svr/sdr_js/src 	\
			./svr/sdr_lc/src			\
			./svr/sdr_lf/src			\
			./svr/sdr_xcch/src			\
			./svr/sdr_xe/src			\
			./svr/sdr_xe96/src			\
			./svr/sdr_xf/src			\
			./svr/sdr_xx96/src			\
			./svr/sdr_yc/src			\
			./svr/sdr_yc_2/src			\
			./svr/sdr_yclj/src			\
			./svr/sdr_ysg/src			\
			./svr/ssp_lj/src			\
			./svr/ssp_ly/src			\
			./svr/wk_lx/src			\
			
svrs :
	for dir in $(SVR_DIR); do 	\
		make -C $${dir} -j2; \
	done
	
clean_svr : 
	for dir in $(SVR_DIR); do 	\
		make clean -C $${dir}; 	\
	done
	rm -f proto/*.pb.*
	
release : svrs

clean : clean_svr
