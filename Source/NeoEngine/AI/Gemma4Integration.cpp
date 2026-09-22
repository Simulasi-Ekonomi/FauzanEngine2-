#include "Gemma4Integration.h"
#include <cstdlib>
#include <fstream>
#include <android/log.h>
#include <algorithm>
#include <limits>
#include <unistd.h>
#define LOG_TAG "Gemma4Int"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
namespace NeoEngine {
Gemma4Integration::Gemma4Integration():ready(false),modelSize(Gemma4ModelSize::Base),modelHandle(nullptr){}
Gemma4Integration::~Gemma4Integration(){Shutdown();}
bool Gemma4Integration::Initialize(Gemma4ModelSize size){if(size!=Gemma4ModelSize::Small&&size!=Gemma4ModelSize::Base){ready=false;return false;}modelSize=size;const std::string root="/sdcard/Gemma4/";modelFilePath=size==Gemma4ModelSize::Small?root+"gemma4_2b_v09_obfus_fix_all_modalities_thinking.litertlm":root+"gemma4_thinking.bin";std::ifstream file(modelFilePath,std::ios::binary);if(!file.good()){ready=false;return false;}file.seekg(0,std::ios::end);if(file.tellg()<=0){ready=false;return false;}file.close();FILE* pipe=popen("command -v gemma 2>/dev/null","r");if(!pipe){ready=false;return false;}char buffer[256];std::string result;while(fgets(buffer,sizeof(buffer),pipe)!=nullptr)result+=buffer;const int status=pclose(pipe);ready=status==0&&!result.empty();LOGI("Gemma CLI %s",ready?"ready":"unavailable");return ready;}
void Gemma4Integration::Shutdown(){ready=false;}
static std::string ShellQuote(const std::string& value){std::string q="'";for(char c:value){if(c=='\'')q+="'\\''";else q+=c;}q+="'";return q;}
static std::pair<std::string,int> execCommand(const std::string& command){std::string result;FILE* pipe=popen(command.c_str(),"r");if(!pipe)return {result,-1};char buffer[1024];while(fgets(buffer,sizeof(buffer),pipe)!=nullptr){if(result.size()>16U*1024U*1024U-sizeof(buffer)) return {std::string{},-1}; result+=buffer;}return {result,pclose(pipe)};}
Gemma4Response Gemma4Integration::GenerateText(const std::string& prompt,int maxLength){Gemma4Response resp{};if(prompt.size()>16U*1024U*1024U)return resp;if(!ready||prompt.empty()||maxLength<=0||maxLength>131072)return resp;const std::string command="gemma --model "+ShellQuote(modelFilePath)+" --prompt "+ShellQuote(prompt)+" --max_tokens "+std::to_string(maxLength);const auto [output,status]=execCommand(command);if(status!=0||output.empty())return resp;if(output.size()>16U*1024U*1024U)return resp;resp.generatedText=output;resp.confidence=0.9f;resp.tokensUsed=static_cast<int>(std::min<size_t>(output.size()/4U,static_cast<size_t>(std::numeric_limits<int>::max())));return resp;}
std::vector<float> Gemma4Integration::GetEmbeddings(const std::string&){return{};}
Gemma4Response Gemma4Integration::Summarize(const std::string& text){if(text.empty())return{};return GenerateText("Summarize: "+text,128);}
bool Gemma4Integration::IsReady()const{return ready;}
std::string Gemma4Integration::GetModelInfo()const{return"Gemma 4 "+std::to_string(static_cast<int>(modelSize))+" (local CLI)";}
void Gemma4Integration::SetModelSize(Gemma4ModelSize size){if(!ready&&(size==Gemma4ModelSize::Small||size==Gemma4ModelSize::Base))modelSize=size;}
}