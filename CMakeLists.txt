project(ohos-napi-framework
	LANGUAGES C CXX
	VERSION 0.1.0.510
	DESCRIPTION "NAPI Wrapper For HarmonyOS"
)

add_library(napi-framework INTERFACE include/napi_framework.h include/napi_framework-inl.h)
target_include_directories(napi-framework INTERFACE include/)
target_link_libraries(napi-framework INTERFACE libace_napi.z.so)
add_library(napi::framework ALIAS napi-framework)

# set(NAPI_AKI_JSBIND aki_jsbind)

if(CAPABLE_WITH_AKI)
	if (NOT DEFINED NAPI_AKI_JSBIND)
		message(FATAL_ERROR "Please specifiy position of your aki library explicitly.")
	endif()
    target_link_libraries(napi-framework INTERFACE ${NAPI_AKI_JSBIND})
    target_compile_definitions(napi-framework INTERFACE CAPABLE_WITH_AKI)
endif()
