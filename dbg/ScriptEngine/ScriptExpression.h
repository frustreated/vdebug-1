/*
调试脚本表达式实现,解析并执行单条调试表达式
*/
#pragma once
#include <Windows.h>
#include <set>
#include <map>
#include <vector>
#include <ComLib/ComLib.h>
#include "ScriptDef.h"
#include "ScriptParser.h"

using namespace std;

typedef VariateDesc *(* pfnProcInternal)(vector<VariateDesc *> &paramSet, void *param);

struct ScriptProcRegisterInfo {
    mstring mProcName;
    VariateType mReturnType;
    vector<VariateType> mParamType;
    pfnProcInternal mProcInternal;

    ScriptProcRegisterInfo() {
        mReturnType = em_var_pending;
        mProcInternal = NULL;
    }
};

class CScriptExpReader {
    #define SCRIPT_DATA_DATA        0
    #define SCRIPT_DATA_OPERATOR    1
    struct ScriptData {
        int mType;          //0: data, 1: operator
        mstring mContent;   //content
    };

    struct ScriptProc 
    {
        mstring mProcStr;   //模式匹配到的函数内容
        size_t mStartPos;   //在源串中的起始位置
        size_t mEndPos;     //在源串中的结束位置
    };

    friend class CScriptParser;
public:
    static CScriptExpReader *GetInst();
    void SetCache(ScriptCache *cache);
    VariateDesc *ParserExpression(const ScriptCmdContext &ctx);

private:
    CScriptExpReader();
    virtual ~CScriptExpReader();

private:
    void InitReader();
    void AddVar(VariateDesc *desc);
    VariateDesc *GetVarByName(const mstring &name) const;
    VariateDesc *ParserStr(const mstring &str);
    size_t GetParamResult(vector<ScriptData> &nodeSet, size_t index);
    VariateDesc *GetDataDesc(const mstring &str);
    //计算表达式中的函数
    void ParserProc(mstring &script);
    //获取字符串中的函数集合
    vector<ScriptProc> GetProcSet(const mstring &express) const;

    //模式识别
    bool IsOperator(const mstring &script, size_t pos, mstring &opt) const;
    bool IsVariable(const mstring &str) const;
    bool IsProc(const mstring &str) const;
    bool IsNumber(const mstring &str) const;
    //gbk字符串判定
    bool IsGbkStr(const mstring &str) const;
    //unicode字符串判定
    bool IsUnicodeStr(const mstring &str) const;
    //是否是内置变量 eg:@esp, @ebp, @eax, @param0, @param1
    bool IsVarInternal(const mstring &str, VariateDesc *&desc);

    VariateDesc *ProcessScriptNode(vector<ScriptData> &nodeSet);
    VariateDesc *ProcessSimpleStr(const mstring &expression);

    VariateDesc *GetGbkDesc(const mstring &str);
    VariateDesc *GetUnicodeDesc(const mstring &str);
    VariateDesc *GetIntDesc(DWORD64 d);
    VariateDesc *GetPtrDesc(LPVOID ptr);
    VariateDesc *GetPendingDesc();
    BOOL GetNumFromStr(const mstring &strNumber, DWORD64 &dwResult) const;
    void ReplaceNode(vector<ScriptData> &nodeSet, size_t pos1, size_t pos2, const ScriptData &newNode) const;
private:
    bool RegisterProc(
        const mstring &procName,
        VariateType returnType,
        const vector<VariateType> &paramSet,
        pfnProcInternal proc
        );
    VariateDesc *CallInternalProc(const mstring &procName, vector<VariateDesc *> &param, void *p = NULL);

    //internal proc
    static VariateDesc *ProcStrStartWithA(vector<VariateDesc *> &paramSet, void *p);
    static VariateDesc *ProcStrStartWithW(vector<VariateDesc *> &paramSet, void *p);
    static VariateDesc *ProcStrSubStrA(vector<VariateDesc *> &paramSet, void *p);
    static VariateDesc *ProcStrSubStrW(vector<VariateDesc *> &paramSet, void *p);
    static VariateDesc *ProcStrCatA(vector<VariateDesc *> &paramSet, void *p);
    static VariateDesc *ProcStrCatW(vector<VariateDesc *> &paramSet, void *p);
    static VariateDesc *ProcRunCommand(vector<VariateDesc *> &paramSet, void *p);

private:
    map<mstring, ScriptProcRegisterInfo*> mProcSet;
    set<mstring> mOperatorSet;
    set<mstring> mCmdInternal;
    ScriptCache *mCache;

    static DWORD msVarSerial;
    map<mstring, VariateDesc *> mTempVarSet;    //临时变量缓存，解析用
    static ThreadPoolBase *sThreadPool;         //表达式执行线程池
};