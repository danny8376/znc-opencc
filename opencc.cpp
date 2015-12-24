/*
 * Copyright (C) 2004-2012  See the AUTHORS file for details.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published
 * by the Free Software Foundation.
 */

#include "znc/User.h"
#include "znc/Chan.h"
#include "znc/znc.h"

#include <map>
#include "opencc/opencc.h"


class COpenCC : public CModule {
public:
    MODCONSTRUCTOR(COpenCC) {
    }

    virtual ~COpenCC() {
#ifdef OPENCC_EXPORT
        for(std::map<CString,Opencc::SimpleConverter*>::iterator i = opencc_ods.begin(); opencc_ods.end() != i ; ++i){
            delete i->second;
        }
#else
        for(std::map<CString,opencc_t>::iterator i = opencc_ods.begin(); opencc_ods.end() != i ; ++i){
            opencc_close(i->second);
        }
#endif
        opencc_ods.clear();
    }

    virtual bool OnLoad(const CString& sArgs, CString& sMessage){
        CString conf_path = sArgs.Token(0);
        if (conf_path.empty()){
            opencc_config_path = "/usr/share/opencc/";
        }else{
            opencc_config_path = conf_path + "/";
        }
        m_ToModes.clear();
        m_FromModes.clear();
        for(MCString::iterator i = BeginNV(); EndNV() !=i ; ++i) {
            if (!i->first.empty() && 't'==i->first[0]) {
                m_ToModes[i->first.substr(1)] = i->second;
            }
            if (!i->first.empty() && 'f'==i->first[0]) {
                m_FromModes[i->first.substr(1)] = i->second;
            }
        }
        return true;
    }

    virtual void OnModCommand(const CString& sLine) {
        CString sCommand = sLine.Token(0);
        sCommand.MakeLower();
        //here to config
        if (sCommand.Equals("set")) {
            CString sTarget = sLine.Token(1);
            if (CString::npos==sTarget.find('/')) {
                if (!sTarget.empty() && '#'==sTarget[0]) {
                    sTarget += "/";
                } else {
                    sTarget = "/" + sTarget;
                }
            }
            CString sFromMode = sLine.Token(2);
            if (sFromMode.empty() || "NONE" == sFromMode) {
                m_FromModes.erase(sTarget);
                DelNV("f" + sTarget);
            } else {
                m_FromModes[sTarget] = sFromMode;
                SetNV("f" + sTarget, sFromMode);
            }
            CString sToMode = sLine.Token(3);
            if (sToMode.empty() || "NONE" == sToMode) {
                m_ToModes.erase(sTarget);
                DelNV("t" + sTarget);
            } else {
                m_ToModes[sTarget] = sToMode;
                SetNV("t" + sTarget, sToMode);
            }
            PutModule("ok");
        } else if (sCommand.Equals("clear")) {
            m_ToModes.clear();
            m_FromModes.clear();
            ClearNV();
            PutModule("ok");
        } else if (sCommand.Equals("list")) {
            CTable Table;
            Table.AddColumn("Channel");
            Table.AddColumn("Nick");
            Table.AddColumn("ToUserMode");
            Table.AddColumn("FromUserMode");
            std::map<CString,CString> sToModes = m_ToModes, sFromModes = m_FromModes;
            for(std::map<CString,CString>::iterator i = m_ToModes.begin(); m_ToModes.end() != i ; ++i){
                std::map<CString,CString>::const_iterator found = sFromModes.find(i->first);
                if (sFromModes.end() == found){
                    sFromModes[i->first] = "NONE";
                }
            }
            for(std::map<CString,CString>::iterator i = m_FromModes.begin(); m_FromModes.end() != i ; ++i){
                std::map<CString,CString>::const_iterator found = sToModes.find(i->first);
                if (sToModes.end() == found){
                    sToModes[i->first] = "NONE";
                }
            }
            for(std::map<CString,CString>::iterator i = sToModes.begin(); sToModes.end() != i ; ++i){
                Table.AddRow();
                CString sChannel = i->first.Token(0, false, "/", true);
                if (sChannel.empty()) {
                    sChannel = "(PM)";
                }
                Table.SetCell("Channel", sChannel);
                Table.SetCell("Nick", i->first.Token(1, false, "/", true));
                Table.SetCell("ToUserMode", i->second);
                Table.SetCell("FromUserMode", sFromModes[i->first]);
            }
            PutModule(Table);
//      } else if (sCommand.Equals("avail")) {
//          PutModule("not implemented");
        } else {
            PutModule("Commands:");
            CTable Table;
            Table.AddColumn("Command");
            Table.AddColumn("Arguments");
            Table.AddColumn("Description");
            
            Table.AddRow();
            Table.SetCell("Command", "my");
            Table.SetCell("Arguments", "<mode>");
            Table.SetCell("Description", "Set your mode");

            Table.AddRow();
            Table.SetCell("Command", "set");
            Table.SetCell("Arguments", "<channel> <from user mode> <to user mode>");
            Table.SetCell("Description", "Set mode of channel (mode NONE means no opencc)");

            Table.AddRow();
            Table.SetCell("Command", "set");
            Table.SetCell("Arguments", "<nick> <from user mode> <to user mode>");
            Table.SetCell("Description", "Set mode of private messages for nick (mode NONE means no opencc)");

            Table.AddRow();
            Table.SetCell("Command", "set");
            Table.SetCell("Arguments", "<channel>/<nick> <from user mode> <to user mode>");
            Table.SetCell("Description", "Set mode for nick on channel (mode NONE means no opencc)");

            Table.AddRow();
            Table.SetCell("Command", "clear");
            Table.SetCell("Description", "Clear all targets");

            Table.AddRow();
            Table.SetCell("Command", "list");
            Table.SetCell("Description", "Show list of configured targets with their languages");

            Table.AddRow();
            Table.SetCell("Command", "help");
            Table.SetCell("Description", "Show this text");

            PutModule(Table);

//          PutModule("avail   - show list of available languages");
        }
    }

#ifdef OPENCC_EXPORT
    Opencc::SimpleConverter* GetOpenCC(CString config){ // opencc - 1.0
        std::map<CString,Opencc::SimpleConverter*>::const_iterator ode = opencc_ods.find(config);
        if (opencc_ods.end() != ode){
            return ode->second;
        }
        try{
            Opencc::SimpleConverter* od = new Opencc::SimpleConverter(opencc_config_path + config + ".json");
        }catch (const runtime_error& error){
            PutModule("Wrong config: " + config);
            return (Opencc::SimpleConverter*)NULL;
        }
        opencc_ods[config] = od;
        return od;
    }
#else
    opencc_t GetOpenCC(CString config){ // opencc - 0.4.3
        std::map<CString,opencc_t>::const_iterator ode = opencc_ods.find(config);
        if (opencc_ods.end() != ode){
            return ode->second;
        }
        opencc_t od = opencc_open((opencc_config_path + config + ".ini").c_str());
        if (od == (opencc_t)-1){
            if (opencc_errno() == OPENCC_ERROR_CONFIG){
                PutModule("Wrong config: " + config);
            }
            return od;
        }
        opencc_ods[config] = od;
        return od;
    }
#endif

    EModRet HandleToUser(const CString& sChannel, const CString& sNick, const CString& sMessage, const CString& sPrefix, const CString& sSuffix){
        std::map<CString,CString>::const_iterator i = m_ToModes.find(sChannel + "/" + sNick);
        if (m_ToModes.end() == i) {
            i = m_ToModes.find(sChannel + "/");
            if (m_ToModes.end() == i) {
                return CONTINUE;
            }
        }

#ifdef OPENCC_EXPORT
        // opencc - 1.0
        Opencc::SimpleConverter* od = GetOpenCC(i->second);
        CString sResult = od->Convert(sMessage);
        PutUser(sPrefix + sResult + sSuffix);
        return HALT;
#else
        // opencc - 0.4.3
        opencc_t od = GetOpenCC(i->second);
        if (od != (opencc_t)-1){
            int length = sMessage.length();
            char* outbuf = opencc_convert_utf8(od, sMessage.c_str(), length);
            if (outbuf != (char*)-1){
                CString sResult = outbuf;
                free(outbuf);
                PutUser(sPrefix + sResult + sSuffix);
                return HALT;
            }
        }
        return CONTINUE;
#endif
    }

    EModRet HandleFromUser(CString sTarget, CString sMessage, CString sPrefix, const CString& sSuffix){
        if (sTarget.empty()) {
            return CONTINUE;
        }
        std::map<CString,CString>::const_iterator i = m_FromModes.end();
        if ('#' == sTarget[0]) {
            sTarget += "/";
            // Evil heuristics to determine, who is receiver of message
            CString sNick = sMessage.Token(0);
            if (!sNick.empty()) {
                sNick.Trim(",:");
                i = m_FromModes.find(sTarget + sNick);
                if (m_FromModes.end() != i) {
                    sPrefix += sMessage.Token(0) + " ";
                    sMessage = sMessage.Token(1, true);
                }
            }
        } else {
            sTarget = "/" + sTarget;
        }
        if (m_FromModes.end() == i) {
            i = m_FromModes.find(sTarget);
            if (m_FromModes.end() == i) {
                return CONTINUE;
            }
        }

#ifdef OPENCC_EXPORT
        // opencc - 1.0
        Opencc::SimpleConverter* od = GetOpenCC(i->second);
        CString sResult = od->Convert(sMessage);
        PutIRC(sPrefix + sResult + sSuffix);
        return HALT;
#else
        // opencc - 0.4.3
        //opencc_t od = opencc_open(("/usr/share/opencc/" + i->second + ".ini").c_str());
        opencc_t od = GetOpenCC(i->second);
        if (od != (opencc_t)-1){
            int length = sMessage.length();
            char* outbuf = opencc_convert_utf8(od, sMessage.c_str(), length);
            if (outbuf != (char*)-1){
                CString sResult = outbuf;
                free(outbuf);
                PutIRC(sPrefix + sResult + sSuffix);
                return HALT;
            }
        }
        return CONTINUE;
#endif
    }

    virtual EModRet OnUserMsg(CString &sTarget, CString &sMessage){
        return HandleFromUser(sTarget, sMessage, "PRIVMSG " + sTarget + " :", "");
    }

    virtual EModRet OnUserNotice(CString &sTarget, CString &sMessage){
        return HandleFromUser(sTarget, sMessage, "NOTICE " + sTarget + " :", "");
    }

    virtual EModRet OnPrivMsg(CNick &Nick, CString &sMessage){
        return HandleToUser("", Nick.GetNick(), sMessage, ":" + Nick.GetNickMask() + " PRIVMSG " + GetUser()->GetNick() + " :", "");
    }

    virtual EModRet OnPrivNotice(CNick &Nick, CString &sMessage){
        return HandleToUser("", Nick.GetNick(), sMessage, ":" + Nick.GetNickMask() + " NOTICE " + GetUser()->GetNick() + " :", "");
    }

    virtual EModRet OnChanMsg(CNick &Nick, CChan &Channel, CString &sMessage){
        return HandleToUser(Channel.GetName(), Nick.GetNick(), sMessage, ":" + Nick.GetNickMask() + " PRIVMSG " + Channel.GetName() + " :", "");
    }

    virtual EModRet OnChanNotice(CNick &Nick, CChan &Channel, CString &sMessage){
        return HandleToUser(Channel.GetName(), Nick.GetNick(), sMessage, ":" + Nick.GetNickMask() + " NOTICE " + Channel.GetName() + " :", "");
    }

    virtual EModRet OnPrivAction(CNick &Nick, CString &sMessage){
        return HandleToUser("", Nick.GetNick(), sMessage, ":" + Nick.GetNickMask() + " PRIVMSG " + GetUser()->GetNick() + " :\01ACTION ", "\01");
    }

    virtual EModRet OnChanAction(CNick &Nick, CChan &Channel, CString &sMessage){
        return HandleToUser(Channel.GetName(), Nick.GetNick(), sMessage, ":" + Nick.GetNickMask() + " PRIVMSG " + Channel.GetName() + " :\01ACTION ", "\01");
    }

    virtual EModRet OnUserAction(CString &sTarget, CString &sMessage){
        return HandleFromUser(sTarget, sMessage, "PRIVMSG " + sTarget + " :\01ACTION ", "\01");
    }

private:
    CString opencc_config_path;
    std::map<CString,CString> m_ToModes, m_FromModes;
#ifdef OPENCC_EXPORT
    std::map<CString,Opencc::SimpleConverter*> opencc_ods; // opencc - 1.0
#else
    std::map<CString,opencc_t> opencc_ods; // opencc - 0.4.3
#endif
};

MODULEDEFS(COpenCC, "OpenCC")

