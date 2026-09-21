#include "HermesIntegration.h"
#include <curl/curl.h>
#include <json/json.h>
#include <android/log.h>
#include <cmath>
#include <limits>
#define LOG_TAG "HermesIntegration"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
namespace NeoEngine {
static size_t WriteCallback(void* contents,size_t size,size_t nmemb,std::string* output){const size_t total=size*nmemb;output->append(static_cast<char*>(contents),total);return total;}
HermesIntegration::HermesIntegration():ready(false),currentModel(HermesModelType::Medium),modelHandle(nullptr),temperature(0.7f),maxTokens(2048){hermesEndpoint="http://localhost:8765/v1/chat/completions";}
HermesIntegration::~HermesIntegration(){Shutdown();}
bool HermesIntegration::Initialize(HermesModelType modelType){currentModel=modelType;CURL* curl=curl_easy_init();if(!curl){ready=false;return false;}curl_easy_setopt(curl,CURLOPT_URL,"http://localhost:8765/health");curl_easy_setopt(curl,CURLOPT_TIMEOUT,2L);const CURLcode res=curl_easy_perform(curl);curl_easy_cleanup(curl);ready=(res==CURLE_OK);LOGI("Hermes integration %s",ready?"connected":"unavailable");return ready;}
void HermesIntegration::Shutdown(){ready=false;}
HermesResponse HermesIntegration::GenerateText(const std::string& prompt,float temp){if(!ready||prompt.empty()||!std::isfinite(temp)||temp<0.0f||temp>2.0f||maxTokens<=0||maxTokens>131072)return{"",0.0f,0,""};CURL* curl=curl_easy_init();if(!curl)return{"",0.0f,0,""};Json::Value body;body["prompt"]=prompt;body["temperature"]=temp;body["max_tokens"]=maxTokens;Json::FastWriter writer;const std::string jsonBody=writer.write(body);std::string responseStr;struct curl_slist* headers=curl_slist_append(nullptr,"Content-Type: application/json");curl_easy_setopt(curl,CURLOPT_URL,hermesEndpoint.c_str());curl_easy_setopt(curl,CURLOPT_POSTFIELDS,jsonBody.c_str());curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,WriteCallback);curl_easy_setopt(curl,CURLOPT_WRITEDATA,&responseStr);curl_easy_setopt(curl,CURLOPT_HTTPHEADER,headers);curl_easy_setopt(curl,CURLOPT_TIMEOUT,60L);const CURLcode res=curl_easy_perform(curl);curl_slist_free_all(headers);curl_easy_cleanup(curl);if(res!=CURLE_OK||responseStr.empty())return{"",0.0f,0,""};Json::Value root;Json::Reader reader;if(!reader.parse(responseStr,root)||!root.isObject()||!root["choices"].isArray()||root["choices"].empty()||!root["choices"][0]["message"]["content"].isString())return{"",0.0f,0,""};const std::string text=root["choices"][0]["message"]["content"].asString();if(text.empty()||text.size()>16U*1024U*1024U)return{"",0.0f,0,""};const int tokens=text.size()/4U>static_cast<size_t>(std::numeric_limits<int>::max())?std::numeric_limits<int>::max():static_cast<int>(text.size()/4U);return{text,0.9f,tokens,"hermes-http"};}
HermesResponse HermesIntegration::Chat(const std::string& msg,int ctx){if(ctx<=0||ctx>131072)return{"",0.0f,0,""};return GenerateText(msg,temperature);}
HermesResponse HermesIntegration::CodeGeneration(const std::string& desc){if(desc.empty())return{"",0.0f,0,""};return GenerateText("Generate C++ game engine code for: "+desc,0.2f);}
void HermesIntegration::SetTemperature(float temp){if(std::isfinite(temp)&&temp>=0.0f&&temp<=2.0f)temperature=temp;}
void HermesIntegration::SetMaxTokens(int max){if(max>0&&max<=131072)maxTokens=max;}
bool HermesIntegration::IsReady()const{return ready;}
std::string HermesIntegration::GetModelInfo()const{return"Hermes Agent (HTTP)";}
}