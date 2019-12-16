/* --------------------------------------------------------------------------------
 #
 #  4DPlugin-Discount.cpp
 #	source generated by 4D Plugin Wizard
 #	Project : Discount
 #	author : miyako
 #	2019/12/16
 #  
 # --------------------------------------------------------------------------------*/

#include "4DPlugin-Discount.h"

#pragma mark -

void PluginMain(PA_long32 selector, PA_PluginParameters params) {
    
	try
	{
        switch(selector)
        {
			// --- Discount
            
			case 1 :
				Markdown(params);
				break;

        }

	}
	catch(...)
	{

	}
}

#pragma mark -

void Markdown(PA_PluginParameters params) {
    
    sLONG_PTR *pResult = (sLONG_PTR *)params->fResult;
    PackagePtr pParams = (PackagePtr)params->fParameters;
    
    C_TEXT Param1;
    C_TEXT Param2;
    C_LONGINT Param3;
    C_LONGINT Param4;
    C_LONGINT returnValue;

    Param1.fromParamAtIndex(pParams, 1);
    Param3.fromParamAtIndex(pParams, 3);
    Param4.fromParamAtIndex(pParams, 4);

    CUTF8String src;
    Param1.copyUTF8String(&src);
    
    mkd_flag_t flags;
    flags = (mkd_flag_t)Param3.getIntValue();
    
    if(MKD_WITH_HTML5_TAGS && Param4.getIntValue())
        mkd_with_html5_tags();
    
    Document *doc;
    
    if(MKD_GITHUB_FLAVOURED && Param4.getIntValue()) {
        doc = gfm_string((const char *)src.c_str(), src.length(), flags);
    }else{
        doc = mkd_string((const char *)src.c_str(), src.length(), flags);
    }
    
    if(doc) {
    
        if(mkd_compile(doc, flags)) {
            
            std::ostringstream outstream;
            
            mkd_generatecss(doc, outstream);
            mkd_generatetoc(doc, outstream);
            mkd_generatehtml(doc, outstream);
            
            std::string dst = outstream.str();
            Param2.setUTF8String((const uint8_t *)dst.c_str(), dst.length());
            
        }else{returnValue.setIntValue(errno);}
        
        mkd_cleanup(doc);
        
    }else{returnValue.setIntValue(errno);}
    
    mkd_deallocate_tags();

    Param2.toParamAtIndex(pParams, 2);
    returnValue.setReturn(pResult);
}

