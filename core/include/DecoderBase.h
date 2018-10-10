#pragma once
 
 class CDecoderBase
 {
	 public:
		 CDecoderBase(){};
		virtual ~CDecoderBase(){};
		virtual int ParsePacket(char * data, int len)=0;
 };



