#include <Windows.h>
#include "mstring.h"
#include "ProcPrinter.h"
#include "ProcParser.h"
#include "StrUtil.h"

CProcPrinter::CProcPrinter() {
}

CProcPrinter::~CProcPrinter() {
}

CProcPrinter *CProcPrinter::GetInst() {
    static CProcPrinter *s_ptr = NULL;

    if (NULL == s_ptr)
    {
        s_ptr = new CProcPrinter();
    }
    return s_ptr;
}

mstring CProcPrinter::GetProcStrByAddr(const mstring &name, LPVOID stackAddr) const {
    return "";
}

mstring CProcPrinter::GetProcStr(const mstring &name) const {
    return "";
}

mstring CProcPrinter::GetStructStrByAddr(const mstring &name, LPVOID startAddr) const {
    return GetStructStrInternal(name, startAddr);
}

mstring CProcPrinter::GetStructStr(const mstring &name) const {
    return GetStructStrInternal(name, NULL);
}

bool IsValidaAddr(LPVOID addr) {
    return (ULONGLONG)addr > 0xffff;
}

void CProcPrinter::StructHandler(PrintEnumInfo &tmp1, list<PrintEnumInfo> &enumSet, bool withOffset) const {
    tmp1.mNode->mSubSize = tmp1.mDesc->mMemberSet.size();

    PrinterNode *lastNode = NULL;
    size_t i = 0;
    for (i = 0 ; i != tmp1.mDesc->mMemberSet.size() ; i++)
    {
        PrintEnumInfo tmp2;
        tmp2.mDesc = tmp1.mDesc->mMemberSet[i];
        tmp2.mNode = new PrinterNode();
        tmp2.mParent = tmp2.mNode;
        tmp2.mNode->mParent = tmp1.mParent;
        tmp1.mParent->mSubNodes.push_back(tmp2.mNode);

        tmp2.mNode->mOffset = FormatA("0x%02x", tmp1.mDesc->mMemberOffset[i]);
        tmp2.mNode->mType = tmp1.mDesc->mMemberType[i];
        tmp2.mNode->mName = tmp1.mDesc->mMemberName[i];

        bool next = true;
        if (!withOffset)
        {
            tmp2.mBaseAddr = (LPVOID)((const char *)tmp1.mBaseAddr + tmp1.mDesc->mMemberOffset[i]);
            tmp2.mNode->mAddr = FormatA("0x%08x", tmp2.mBaseAddr);

            bool isUnicode = false;
            if (tmp2.mDesc->mType == STRUCT_TYPE_PTR && tmp2.mDesc->IsStr(isUnicode))
            {
                char *pStr = *((char **)tmp2.mBaseAddr);
                if (!pStr)
                {
                    tmp2.mNode->mContent = "读取地址内容错误";
                } else {
                    if (isUnicode)
                    {
                        tmp2.mNode->mContent = FormatA("%ls", pStr);
                    } else {
                        tmp2.mNode->mContent = FormatA("%hs", pStr);
                    }
                }
                next = false;
            } else {
                if (tmp2.mDesc->mType != STRUCT_TYPE_STRUCT)
                {
                    if (IsValidaAddr(tmp2.mBaseAddr))
                    {
                        tmp2.mNode->mContent = tmp2.mDesc->mPfnFormat(tmp2.mBaseAddr, tmp2.mDesc->mLength);
                    } else {
                        tmp2.mNode->mContent = "读取地址内容错误";
                    }
                }
            }
        }

        if (lastNode != NULL)
        {
            lastNode->mNextBrotherNode = tmp2.mNode;
            tmp2.mNode->mLastBrotherNode = lastNode;
        }
        lastNode = tmp2.mNode;

        if (next)
        {
            enumSet.push_back(tmp2);
        }
    }
}

//生成节点层次结构
PrinterNode *CProcPrinter::GetNodeStruct(const mstring &name, LPVOID baseAddr) const {
    StructDesc *desc = CProcParser::GetInst()->FindStructFromName(name);
    if (NULL == desc)
    {
        return NULL;
    }

    PrinterNode *root = new PrinterNode();
    list<PrintEnumInfo> enumSet;
    PrintEnumInfo tmp;
    tmp.mParent = root;
    tmp.mDesc = desc;
    tmp.mNode = root;
    tmp.mBaseAddr = baseAddr;

    root->mName = name;
    bool withOffset = (baseAddr == NULL);
    enumSet.push_back(tmp);

    int i = 0;
    while (!enumSet.empty()) {
        PrintEnumInfo tmp1 = enumSet.front();
        enumSet.pop_front();

        if (tmp1.mDesc->mType == STRUCT_TYPE_STRUCT)
        {
            StructHandler(tmp1, enumSet, withOffset);
        } else if ((tmp1.mDesc->mType == STRUCT_TYPE_PTR) && (tmp1.mDesc->mUnknownType == false))
        {
            PrintEnumInfo tmp3;
            tmp3.mDesc = tmp1.mDesc->mPtr;
            tmp3.mNode = new PrinterNode();
            tmp3.mParent = tmp1.mParent;

            if (NULL == tmp1.mBaseAddr)
            {
                tmp3.mNode->mParent = tmp3.mParent;
                tmp3.mNode->mContent = "读取地址内容错误";
            } else {
                char *pStr = *((char **)tmp1.mBaseAddr);
                tmp3.mBaseAddr = pStr;

                if (tmp3.mDesc->mType == STRUCT_TYPE_STRUCT)
                {
                    StructHandler(tmp3, enumSet, withOffset);
                }
            }
        }
    }
    return root;
}

void CProcPrinter::FillLineAndRow(PrinterNode *ptr, vector<PrinterNode *> &result, int &line) const {
    if (ptr->mLastBrotherNode != NULL)
    {
        ptr->mRow = ptr->mLastBrotherNode->mRow;
    } else {
        if (ptr->mParent != NULL)
        {
            ptr->mRow = ptr->mParent->mRow + 1;
        }
    }

    ptr->mLine = line++;
    result.push_back(ptr);

    for (list<PrinterNode *>::iterator it = ptr->mSubNodes.begin() ; it != ptr->mSubNodes.end() ; it++)
    {
        PrinterNode *ptr2 = *it;
        FillLineAndRow(ptr2, result, line);
    }
}

void CProcPrinter::LinkDetachedNode(const vector<PrinterNode *> &nodeSet, vector<mstring> &strSet) const {
    for (size_t i = 0 ; i < strSet.size() ; i++)
    {
        PrinterNode *ptr = nodeSet[i + 1];

        if (ptr->mLastBrotherNode && (ptr->mLastBrotherNode->mLine + 1) < ptr->mLine)
        {
            for (int j = ptr->mLastBrotherNode->mLine ; j < ptr->mLine - 1 ; j++)
            {
                mstring last = strSet[i];
                size_t pos1 = last.find("└");
                if (mstring::npos == pos1)
                {
                    pos1 = last.find("├");
                }

                mstring dd = strSet[j];
                dd.replace(pos1, 1, "│");
                strSet[j] = dd;
            }
        }
    }
}

mstring CProcPrinter::GetStructStrInternal(const mstring &name, LPVOID baseAddr) const {
    PrinterNode *root = GetNodeStruct(name, baseAddr);
    vector<PrinterNode *> lineSet;
    int line = 0;

    FillLineAndRow(root, lineSet, line);

    mstring result;
    int offset = 0;
    bool withOffset = (baseAddr == NULL);

    if (withOffset)
    {
        offset = lstrlenA("└─0x12");
    } else {
        offset = lstrlenA("└─0x1122abcd");
    }

    size_t i = 0;
    vector<mstring> fmtSet;
    for (i = 1 ; i < lineSet.size() ; i++) {
        PrinterNode *ptr = lineSet[i];

        if (!withOffset)
        {
            fmtSet.push_back(FormatA("%hs %hs %hs %hs", ptr->mAddr.c_str(), ptr->mType.c_str(), ptr->mName.c_str(), ptr->mContent.c_str()));
        } else {
            fmtSet.push_back(FormatA("%hs %hs %hs", ptr->mOffset.c_str(), ptr->mType.c_str(), ptr->mName.c_str()));
        }
    }

    PrinterNode *ptr = NULL;
    for (i = 0 ; i < fmtSet.size() ; i++)
    {
        ptr = lineSet[i + 1];

        int curOffset = (ptr->mRow - 1) * offset;
        mstring dd;
        for (int j = 0 ; j < curOffset ; j++)
        {
            dd += " ";
        }

        if (ptr->mNextBrotherNode)
        {
            dd += "├─";
        } else {
            dd += "└─";
        }
        dd += fmtSet[i];
        fmtSet[i] = dd;
    }

    LinkDetachedNode(lineSet, fmtSet);
    result = (lineSet.front()->mName + ":\n");
    for (vector<mstring>::const_iterator ij = fmtSet.begin() ; ij != fmtSet.end() ; ij++)
    {
        result += (*ij + "\n");
    }
    return result;
}