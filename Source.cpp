#define WINVER 0x0502
#include <windows.h>
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstring>
#include <sddl.h>

using namespace std;

//���������� ��������� SECURITY_ATRIBUTES

static PSECURITY_DESCRIPTOR create_security_descriptor()
{
   const char* sddl ="D:(A;OICI;GRGW;;;AU)(A;OICI;GA;;;BA)";
   PSECURITY_DESCRIPTOR security_descriptor = NULL;
   ConvertStringSecurityDescriptorToSecurityDescriptor(sddl, SDDL_REVISION_1, &security_descriptor, NULL);
   return security_descriptor;
}

static SECURITY_ATTRIBUTES create_security_attributes()
{
    SECURITY_ATTRIBUTES attributes;
    attributes.nLength = sizeof(attributes);
    attributes.lpSecurityDescriptor = create_security_descriptor();
    attributes.bInheritHandle = FALSE;
    return attributes;
}

// 1.�������� ��������� �����:

BOOL WINAPI Create_MailSlot(LPCTSTR lpName, HANDLE& hslot, bool& process)
{
    auto Attributes = create_security_attributes();
    hslot = CreateMailslot(lpName,NULL,MAILSLOT_WAIT_FOREVER,&Attributes);

    if (hslot == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();

        if (error == ERROR_INVALID_NAME||error == ERROR_ALREADY_EXISTS)
        {
            process = FALSE;
            hslot = CreateFile(lpName,GENERIC_WRITE,FILE_SHARE_READ,(LPSECURITY_ATTRIBUTES)NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,(HANDLE)NULL);

            if (hslot == INVALID_HANDLE_VALUE)
            {
                DWORD error = GetLastError();
                printf("������ ��� ���������� ������� CreateFile().���: %d\n", GetLastError());
                return FALSE;
            }
        }
        else
        {
            printf("������ ��� ���������� ������� CreateFile().���: %d\n", GetLastError());
            return FALSE;
        }
    }
    else
        process = TRUE;
    return TRUE;
}

// 2.1. ������� ��������� ���������� � �������� �����:

BOOL Get_Mailslot_Info(HANDLE hSlot, DWORD* cbMessage, DWORD* cMessage)
{
    DWORD cbMessageL, cMessageL;
    BOOL fresult;

    fresult = GetMailslotInfo(hSlot,(LPDWORD) NULL,&cbMessageL,&cMessageL,(LPDWORD) NULL);

    if (fresult)
    {
        printf("������� %i ��������� �� �������� �����\n", cMessageL);
        if (cbMessage != NULL) 
            *cbMessage = cbMessageL;
        if (cMessage != NULL) 
            *cMessage = cMessageL;
        return TRUE;
    }
    else
    {
        printf("������ ��� ���������� ������� GetMailslotInfo().������: %d\n", GetLastError());
        return FALSE;
    }
}
// 2.2. ������� ������ ���������:
BOOL Write(HANDLE hFile, LPCTSTR lpszMessage)
{
    BOOL hwrite;
    DWORD cbWritten;

    hwrite = WriteFile(hFile,lpszMessage,(lstrlen(lpszMessage) + 1) * sizeof(TCHAR),&cbWritten,(LPOVERLAPPED)NULL);

    if (hwrite!=0)
    {
        printf("��������� ������� ����������!\n");
        return TRUE;
    }
    else
    {
        printf("������ ��� ���������� ������� WriteFile().������: %d\n", GetLastError());
        return FALSE;
    }
    return TRUE;
}

//2.3. ������� ������ ���������:

BOOL Read(HANDLE hFile)
{
    DWORD lpNextSize, lpMessageCount, lpNumberOfBytesRead;
    BOOL fResult;

    lpNextSize = lpMessageCount = lpNumberOfBytesRead = 0;

    fResult = GetMailslotInfo(hFile,NULL,&lpNextSize,&lpMessageCount,NULL);

    if (lpNextSize == MAILSLOT_NO_MESSAGE)
    {
        printf("��� ��������� � �������� �����!\n");
        return TRUE;
    }

    if (fResult==0)
    {
        printf("������ ��� ���������� ������� GetMailslotInfo.������: %d\n", GetLastError());
        return FALSE;
    }

    string Message(lpNextSize,'\0');
    DWORD nNumberOfBytesToRead = sizeof(Message);

    fResult = ReadFile(hFile,&Message[0], nNumberOfBytesToRead,&lpNumberOfBytesRead,NULL);

    if (fResult == 0)
    {
        printf("������ ��� ������� ��������� ���������� �����.������: %d\n", GetLastError());
        return FALSE;
    }
    cout << Message;
    return TRUE;
}

// �������� ���������:

int main()
{
    DWORD* cbMessage;
    DWORD* cMessage;
    cbMessage = cMessage = 0;
    string Name_mailslot;

    setlocale(LC_ALL, "Russian");

    cout << "������� ������������ ��������� �����:";
    cin >> Name_mailslot;

    LPCTSTR SLOT_NAME = Name_mailslot.c_str();

    HANDLE hSlot;
    bool process;

    bool result=Create_MailSlot(SLOT_NAME, hSlot, process);
    if (result==FALSE) 
        return 0;

    printf("������� �������-");
    printf((process) ? "������\n" : "������\n");
    printf("������ ������:\n[check] - ��������� ���������� � ���������� ���������\n");
    printf((process) ? "[read] - ������ ���������� ���������\n" : "[write] - ������ ���������\n");
    printf("[quit] - ���������� ������ ���������\n");

    string command;
    while (1)
    {
        cout << ">";
        cin >> command;
        if (command == "check")
        {
            Get_Mailslot_Info(hSlot, cbMessage, cMessage);
        }
        else if (command == "quit")
        {
            CloseHandle(hSlot);
            return 0;
        }
        else if ((command == "read") && (process))
        {
            Read(hSlot);
        }
        else if ((command == "write") && (!process))
        {
            printf("������� ����� ���������.���� �����������, ���� ������ �� ������.\n");
            
            string message, str;

            getline(cin, str);
             do
            {
                getline(cin, str);
                message += str + '\n';
            } while (!str.empty());
            
            Write(hSlot, (LPCTSTR)message.c_str());
        }
        else if ((command != "write") || (command != "check") || (command != "read") || (command == "quit"))
        {
            printf("�� ����������� ����� �������.����������, ������� ���� �� ������, ������������ � ������:\n[check]-��������� ���������� � ���������� ���������\n");
            printf((process) ? "[read]-������ ���������� ���������\n" : "[write]-������ ���������\n");
            printf("[quit]-���������� ������ ���������\n");
        }
    }
    return 0;
}
