// ChatServer-Week08.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <winsock2.h>
#include <stdio.h>
#pragma comment(lib, "ws2_32")
#pragma warning(disable:4996)
//Khoi tao một struct client
typedef struct {
    SOCKET client;
    char* id;
} CLIENT;

CLIENT clients[256];
int numClients = 0;

void sendNotifications(SOCKET client, char* id, const char* type)
{
    char notification[256];
    sprintf(notification, "\n[%s] %s\n\n", type, id);
    // Gui tin nhan toi client khac
    for (int i = 0; i < numClients; i++)
        if (clients[i].client != client)
            send(clients[i].client, notification, strlen(notification), 0);
}

void removeClient(SOCKET client)
{
    int i = 0;

    for (; i < numClients; i++)
        if (clients[i].client == client) break;

    // Xoa client khoi mang
    if (i < numClients - 1)
        clients[i] = clients[numClients - 1];
    numClients--;
}

void sendMessage(SOCKET client, char* id, char* buf) {
    char tag[32], sendBuf[512];
    // Tach phan tag o sau SEND
    int ret = sscanf(buf + strlen("SEND"), "%s", tag);

    if (ret < 1) {
        const char* errorMsg = "\n[SEND] ERROR Sai cu phap.\n\n";
        send(client, errorMsg, strlen(errorMsg), 0);
        return;
    }

    // Lay tin nhan cua client
    char* msg = buf + strlen("SEND") + strlen(tag) + 2;

    // Gui toi tat ca - ALL
    if (strcmp(tag, "ALL") == 0)
    {
        sprintf(sendBuf, "\n[MESSAGE_ALL] %s: %s\n\n", id, msg);
        for (int i = 0; i < numClients; i++)
            if (clients[i].client != client)
                send(clients[i].client, sendBuf, strlen(sendBuf), 0);
    }
    else {
        if (strcmp(tag, id) == 0)
        {
            const char* errorMsg = "\n[SEND] ERROR ID nhan phai khac chinh minh.\n\n";
            send(client, errorMsg, strlen(errorMsg), 0);
            return;
        }

        int i = 0;
        for (; i < numClients; i++)
            if (strcmp(clients[i].id, tag) == 0)
                break;

        if (i < numClients) {
            sprintf(sendBuf, "\n[MESSAGE] %s: %s\n\n", id, msg);
            send(clients[i].client, sendBuf, strlen(sendBuf), 0);
        }
        else {
            const char* errorMsg = "\n[SEND] ERROR ID nhan khong ton tai.\n\n";
            send(client, errorMsg, strlen(errorMsg), 0);
        }
    }
}

void getListUser(SOCKET client, char* buf) {
    char temp[256];
    int ret = sscanf(buf + strlen("LIST"), "%s", temp);

    if (ret != -1)
    {
        const char* errorMsg = "\n[LIST] ERROR Sai cu phap.\n\n";
        send(client, errorMsg, strlen(errorMsg), 0);
    }
    else {
        const char* msg = "\n[LIST] OK: ";
        send(client, msg, strlen(msg), 0);
        for (int i = 0; i < numClients; i++)
        {
            send(client, clients[i].id, strlen(clients[i].id), 0);
            if (i != numClients - 1) send(client, ", ", strlen(", "), 0);
            else send(client, "\n\n", strlen("\n\n"), 0);
        }
    }
}

bool disconnect(SOCKET client, char* buf, char* id) {
    char temp[256];
    int ret = sscanf(buf + strlen("DISCONNECT"), "%s", temp);

    if (ret != -1)
    {
        const char* errorMsg = "\nDISCONNECT ERROR Sai cu phap.\n\n";
        send(client, errorMsg, strlen(errorMsg), 0);
        return false;
    }
    else {
        removeClient(client);
        sendNotifications(client, id, "USER_DISCONNECT");
        return true;
    }
}

bool isValidID(char* id) {
    // Kiem tra id da ton tai hay chua?
    int i = 0;
    for (; i < numClients; i++) {
        if (strcmp(clients[i].id, id) == 0) break;
    }

    // Da ton tai id
    if (i < numClients) return false;
    return true;
}

DWORD WINAPI ClientThread(LPVOID lpParam)
{
    SOCKET client = *(SOCKET*)lpParam;

    
label_login:

    int ret;
    char buf[256];
    char cmd[32], tag[32], id[32], tmp[32];

    // Xu ly dang nhap
    while (1)
    {
        // Gui thong bao cu phap dang nhap
        const char* cmdMsg = "\nCu phap dang nhap 'CONNECT your_id': ";
        send(client, cmdMsg, strlen(cmdMsg), 0);

        // Nhan cu phap dang nhap tu client
        ret = recv(client, buf, sizeof(buf), 0);
        if (ret <= 0)
            return 0;

        // Loai bo ki tu \n ra khoi buf
        buf[ret - 1] = 0;
        printf("\nDu lieu nhan duoc: %s\n", buf);
        ret = sscanf(buf, "%s %s %s", cmd, id, tmp);

        if (ret != 2)
        {
            const char* errorMessage = "\n[CONNECT] ERROR Cu phap dang nhap khong hop le.\n";
            send(client, errorMessage, strlen(errorMessage), 0);
        }
        else
        {
            // Sai lenh CONNECT
            if (strcmp(cmd, "CONNECT") != 0)
            {
                const char* errorMsg = "\n[CONNECT] ERROR Sai lenh 'CONNECT'.\n";
                send(client, errorMsg, strlen(errorMsg), 0);
            }
            // Dung cu phap thi ta kiem tra id da ton tai hay chua
            else
            {
                if (!isValidID(id))
                {
                    const char* errorMsg = "\n[CONNECT] ERROR ID da ton tai. Dang nhap that bai.\n";
                    send(client, errorMsg, strlen(errorMsg), 0);
                }
                else // Hop le them client hien tai vao mang 
                {
                    // Gui tin nhan dang nhap thanh cong
                    const char* successMsg = "\n[CONNECT] OK Dang nhap thanh cong.\n\n";
                    send(client, successMsg, strlen(successMsg), 0);

                    // Them client vao ds client
                    clients[numClients].id = id;
                    clients[numClients].client = client;
                    numClients++;

                    // Gui thong bao dang nhap toi client khac
                    sendNotifications(client, id, "USER_CONNECT");

                    break;
                }
            }
        }
    }

    // Thuc hien lenh tu client
    while (1)
    {
        // Gui thong bao cu phap dang nhap
        /*const char* cmdMsg = "Nhap yeu cau: ";
        send(client, cmdMsg, strlen(cmdMsg), 0);*/

        ret = recv(client, buf, sizeof(buf), 0);
        if (ret <= 0)
        {
            sendNotifications(client, id, "USER_DISCONNECT");
            removeClient(client);
            closesocket(client);
            return 0;
        }

        // Loai bo ki tu '\n' ra khoi buf
        buf[ret - 1] = 0;
        printf("Du lieu nhan duoc: %s\n", buf);

        // Tach lenh tu buf
        sscanf(buf, "%s", cmd);

        if (strcmp(cmd, "SEND") == 0)
        {
            sendMessage(client, id, buf);
        }
        else if (strcmp(cmd, "LIST") == 0)
        {
            getListUser(client, buf);
        }
        else if (strcmp(cmd, "DISCONNECT") == 0)
        {
            if (disconnect(client, buf, id))
            {
                const char* errorMsg = "\n[DISCONNECT] OK Dang xuat thanh cong.\n\n";
                send(client, errorMsg, strlen(errorMsg), 0);
                goto label_login;
            }
        }
        else
        {
            const char* errorMsg = "\n[ERROR] Lenh khong ton tai.\n\n";
            send(client, errorMsg, strlen(errorMsg), 0);
        }
    }

    closesocket(client);
}

int main()
{
    // Khoi tao thu vien
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Tao socket
    SOCKET listener = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    // Khai bao dia chi server
    SOCKADDR_IN addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(8000);

    // Gan cau truc dia chi voi socket
    bind(listener, (SOCKADDR*)&addr, sizeof(addr));

    // Chuyen sang trang thai cho ket noi
    listen(listener, 5);

    while (1)
    {
        SOCKET client = accept(listener, NULL, NULL);
        printf("\nClient moi ket noi: %d\n", client);
        CreateThread(0, 0, ClientThread, &client, 0, 0);
    }

    return 0;
}

// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
