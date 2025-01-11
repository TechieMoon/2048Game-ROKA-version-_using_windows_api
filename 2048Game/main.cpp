#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <tchar.h>
#include "resource.h"
#include <stdlib.h> // rand(), srand()
#include <time.h>   // time()
#include <math.h> // pow()
#include <stdio.h> // st_printf_s()
#include <mmsystem.h>

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
HINSTANCE g_hInst;
HWND hWndMain;
TCHAR server_ip[128] = _T("127.0.0.1");
TCHAR nickname[128] = _T("");

// ���̾�α� �޽��� ó���� ���� ���� �Լ� ����
BOOL CALLBACK SeverDlgProc(HWND, UINT, WPARAM, LPARAM);
BOOL CALLBACK NameRegisterDlgProc(HWND, UINT, WPARAM, LPARAM);
HWND hDlgMain;

LPCTSTR lpszClass = _T("2048 Game");
enum mode { SINGLE, MULTI };
enum chosen { CHOOSE_SERVER, CHOOSE_CLIENT } server_client;
HBITMAP hBit[17];
int score;

// �Լ� ���� ����
void CopyGameBoard(int copied_board[4][4], int original_board[4][4]);
void MoveLeft(int board[4][4], int move[4][4][2]);
void MoveRight(int board[4][4], int move[4][4][2]);
void MoveUp(int board[4][4], int move[4][4][2]);
void MoveDown(int board[4][4], int move[4][4][2]);
int CompareGameBoard(int board1[4][4], int board2[4][4]);
void DrawBitmap(HDC hdc, int x, int y, int value);
void DrawBlocks(HDC hdc, int x, int y, int board[4][4]);
int GenerateBlock(int board[4][4]);
void DrawMovingBlocks(HDC hdc, int before_board[4][4], int after_board[4][4], int move[4][4][2], int count);
void RegisterRank();
void show_Rank();
// �Լ� ���� ��

int APIENTRY WinMain(_In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance,
	_In_ LPSTR lpszCmdParam, _In_ int nCmdShow)
{
	srand(time(NULL)); // �õ� �ʱ�ȭ (���� �ð��� �������)

	HWND hWnd;
	MSG Message;
	WNDCLASS WndClass;
	g_hInst = hInstance;

	WndClass.cbClsExtra = 0;
	WndClass.cbWndExtra = 0;
	WndClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
	WndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	WndClass.hInstance = hInstance;
	WndClass.lpfnWndProc = WndProc;
	WndClass.lpszClassName = lpszClass;
	WndClass.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
	WndClass.style = CS_HREDRAW | CS_VREDRAW;
	RegisterClass(&WndClass);

	hWnd = CreateWindow(lpszClass, lpszClass, WS_OVERLAPPEDWINDOW,
		300, 100, 1000, 700,
		NULL, (HMENU)NULL, hInstance, NULL);
	ShowWindow(hWnd, nCmdShow);
	// hWndMain=hWnd; // hWnd ������ ���������� ����!

	while (GetMessage(&Message, NULL, 0, 0)) {
		TranslateMessage(&Message);
		DispatchMessage(&Message);
	}
	return (int)Message.wParam;
}

// ��Ƽ�÷��� ������ ���� ��ȭ����
BOOL CALLBACK SeverDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage) {
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_SERVER_IP, server_ip);
		CheckRadioButton(hDlg, IDC_MAKEROOM, IDC_SEARCHROOM, IDC_MAKEROOM);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			if (IsDlgButtonChecked(hDlg, IDC_MAKEROOM) == BST_CHECKED) server_client = CHOOSE_SERVER; // �� ����� ��ư�� ��������
			if (IsDlgButtonChecked(hDlg, IDC_SEARCHROOM) == BST_CHECKED) server_client = CHOOSE_CLIENT; // �� ã�� ��ư�� ��������
			GetDlgItemText(hDlg, IDC_SERVER_IP, server_ip, 128); // ���� �ּ� ���� ����
			EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

// ���� ����� ���� ��ȭ����
BOOL CALLBACK NameRegisterDlgProc(HWND hDlg, UINT iMessage, WPARAM wParam, LPARAM lParam)
{
	switch (iMessage) {
	case WM_INITDIALOG:
		SetDlgItemText(hDlg, IDC_NICKNAME, nickname);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDOK:
			GetDlgItemText(hDlg, IDC_NICKNAME, nickname, 128); // �г��� ���� ����
			EndDialog(hDlg, IDOK);
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT iMessage,
	WPARAM wParam, LPARAM lParam)
{
	HDC hdc;
	PAINTSTRUCT ps;
	static int game_board[4][4];  // ������ �ʱ�ȭ
	static int board_move[4][4][2]; // �� ��ϵ��� ��ŭ �������� �ϴ���
	static int before_move[4][4]; // �����̱� �� ������ ���
	static mode game_mode = SINGLE;
	static int count = 0; // 10�����ӿ� �����̱� ���� ���� ����
	TCHAR str[128];
	static RECT restart_button = { 200, 25, 300, 55 };
	static RECT rank_button = { 400, 25, 500, 55 };
	POINT p;
	static int restart_button_flag = 0;
	static int rank_button_flag = 0;

	switch (iMessage) {
	case WM_CREATE:
		InvalidateRect(hWnd, NULL, TRUE);
		hWndMain = hWnd;
		score = 0;
		for (int i = 0; i < 4; i++)
		{
			for (int j = 0; j < 4; j++)
			{
				game_board[i][j] = 0;
				before_move[i][j] = 0;
				for (int k = 0; k < 2; k++)
				{
					board_move[i][j][k] = 0;
				}
			}
		}
		game_board[0][1] = 1;
		for (int i = 0; i < 17; i++) {
			hBit[i] = LoadBitmap(g_hInst, MAKEINTRESOURCE(IDB_BLOCK1 + i));
		}
		return 0;
	case WM_PAINT:
		hdc = BeginPaint(hWnd, &ps);
		DrawBlocks(hdc, 70, 150, game_board);
		_stprintf_s(str, _T("score: %4d"), score);
		TextOut(hdc, 30, 30, str, _tcslen(str));
		if (restart_button_flag == 1)
			Rectangle(hdc, 200 - 5, 25 - 5, 300 + 5, 55 + 5);
		else
			Rectangle(hdc, 200, 25, 300, 55);
		TextOut(hdc, 230, 30, _T("restart"), 7);
		if (rank_button_flag == 1)
			Rectangle(hdc, 400 - 5, 25 - 5, 500 + 5, 55 + 5);
		else
			Rectangle(hdc, 400, 25, 500, 55);
		TextOut(hdc, 435, 30, _T("rank"), 4);
		EndPaint(hWnd, &ps);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case ID_SELECTGAME_SINGLEPLAY: // �̱� �÷���
			game_mode = SINGLE;
			break;
		case ID_SELECTGAME_MULTIPLAY: // ��Ƽ �÷���
			if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, (DLGPROC)SeverDlgProc) == IDOK)
				game_mode = MULTI;
			break;
		}
		return 0;
	case WM_MOUSEMOVE:
		// ���콺 Ŭ�� ��ġ ����
		p.x = LOWORD(lParam);
		p.y = HIWORD(lParam);
		if (PtInRect(&restart_button, p)) {
			
			if (restart_button_flag == 0)
			{
				sndPlaySound(_T("ButtonSound.wav"), SND_ASYNC);
				restart_button_flag = 1;
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
		else if (PtInRect(&rank_button, p)) {
			if (rank_button_flag == 0)
			{
				sndPlaySound(_T("ButtonSound.wav"), SND_ASYNC);
				rank_button_flag = 1;
				InvalidateRect(hWnd, NULL, TRUE);
			}
		}
		else
		{
			restart_button_flag = 0;
			rank_button_flag = 0;
		}
		return 0;
	case WM_LBUTTONDOWN:
		// ���콺 Ŭ�� ��ġ ����
		p.x = LOWORD(lParam);
		p.y = HIWORD(lParam);

		if (PtInRect(&restart_button, p)) {
			SendMessage(hWnd, WM_CREATE, 0, 0);
		}
		if (PtInRect(&rank_button, p)) {
			show_Rank();
		}
		return 0;
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_LEFT:
			sndPlaySound(_T("MoveSound.wav"), SND_ASYNC);
			CopyGameBoard(before_move, game_board);
			MoveLeft(game_board, board_move);
			SetTimer(hWnd, 1, 15, NULL);
			if (GenerateBlock(game_board) == 0)
			{
				sndPlaySound(_T("GameOver.wav"), SND_ASYNC);
				if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, (DLGPROC)NameRegisterDlgProc) == IDOK)
					RegisterRank();
				if (MessageBox(hWnd, _T("Retry?"), _T("Game Over"), MB_YESNO | MB_ICONQUESTION) == IDNO)
					DestroyWindow(hWnd);
				else
					SendMessage(hWnd, WM_CREATE, 0, 0);
			}
			break;
		case VK_RIGHT:
			sndPlaySound(_T("MoveSound.wav"), SND_ASYNC);
			CopyGameBoard(before_move, game_board);
			MoveRight(game_board, board_move);
			SetTimer(hWnd, 1, 15, NULL);
			if (GenerateBlock(game_board) == 0)
			{
				sndPlaySound(_T("GameOver.wav"), SND_ASYNC);
				if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, (DLGPROC)NameRegisterDlgProc) == IDOK)
					RegisterRank();
				if (MessageBox(hWnd, _T("Retry?"), _T("Game Over"), MB_YESNO | MB_ICONQUESTION) == IDNO)
					DestroyWindow(hWnd);
				else
					SendMessage(hWnd, WM_CREATE, 0, 0);
			}
			break;
		case VK_UP:
			sndPlaySound(_T("MoveSound.wav"), SND_ASYNC);
			CopyGameBoard(before_move, game_board);
			MoveUp(game_board, board_move);
			SetTimer(hWnd, 1, 15, NULL);
			if (GenerateBlock(game_board) == 0)
			{
				sndPlaySound(_T("GameOver.wav"), SND_ASYNC);
				if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, (DLGPROC)NameRegisterDlgProc) == IDOK)
					RegisterRank();
				if (MessageBox(hWnd, _T("Retry?"), _T("Game Over"), MB_YESNO | MB_ICONQUESTION) == IDNO)
					DestroyWindow(hWnd);
				else
					SendMessage(hWnd, WM_CREATE, 0, 0);
			}
			break;
		case VK_DOWN:
			sndPlaySound(_T("MoveSound.wav"), SND_ASYNC);
			CopyGameBoard(before_move, game_board);
			MoveDown(game_board, board_move);
			SetTimer(hWnd, 1, 15, NULL);
			if (GenerateBlock(game_board) == 0)
			{
				sndPlaySound(_T("GameOver.wav"), SND_ASYNC);
				if (DialogBox(g_hInst, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, (DLGPROC)NameRegisterDlgProc) == IDOK)
					RegisterRank();
				if (MessageBox(hWnd, _T("Retry?"), _T("Game Over"), MB_YESNO | MB_ICONQUESTION) == IDNO)
					DestroyWindow(hWnd);
				else
					SendMessage(hWnd, WM_CREATE, 0, 0);
			}
			break;
		}
		return 0;
	case WM_TIMER:
		switch (wParam) {
		case 1:
			hdc = GetDC(hWnd);
			count++;
			DrawMovingBlocks(hdc, before_move, game_board, board_move, count);

			if (count >= 10)
			{
				count = 0;
				KillTimer(hWnd, 1);
				InvalidateRect(hWnd, NULL, TRUE);
				for (int i = 0; i < 4; i++)
					for (int j = 0; j < 4; j++)
					{
						board_move[i][j][0] = 0;
						board_move[i][j][1] = 0;
					}
				return 0;
			}
			DeleteDC(hdc);
			break;
		}
		return 0;
	case WM_DESTROY:
		for (int i = 0; i < 17; i++) {
			DeleteObject(hBit[i]);
		}
		PostQuitMessage(0);
		return 0;
	}
	return(DefWindowProc(hWnd, iMessage, wParam, lParam));
}

void show_Rank() {
	TCHAR name[128];
	int num, rank_score;
	struct Rank {
		TCHAR name[128];
		int score;
	};
	struct Rank rank_array[5] = { 0 };
	int count = 0;

	FILE* file = _tfopen(_T("rank.txt"), _T("r"));
	if (file) {
		while (_ftscanf(file, _T("%d. %127s %d\n"), &num, name, &rank_score) == 3) {
			rank_array[count].score = rank_score;
			_tcscpy_s(rank_array[count].name, name);
			count++;
			if (count >= 5) // �ִ� 5�������� �д´�.
				break;
		}
		fclose(file);
	}

	// 5����� ����� ���ڿ��� �����
	TCHAR rank_to_five[1000] = _T(""); // ���ڿ� ���� �ʱ�ȭ
	for (int i = 0; i < count; i++) {
		_stprintf_s(rank_to_five + _tcslen(rank_to_five),
			1000 - _tcslen(rank_to_five),
			_T("%d. %s %d\n"),
			i + 1, rank_array[i].name, rank_array[i].score);
	}

	MessageBox(NULL, rank_to_five, _T("Rank(5�������)"), MB_OK);
}

void RegisterRank()
{
	TCHAR name[128];
	int num, rank_score;
	struct Rank {
		TCHAR name[128];
		int score;
	};
	struct Rank rank_array[100] = { 0 };
	int count = 0;

	FILE* file = _tfopen(_T("rank.txt"), _T("r"));
	if (file) {
		while (_ftscanf(file, _T("%d. %127s %d\n"), &num, name, &rank_score) == 3) {
			rank_array[count].score = rank_score;
			_tcscpy_s(rank_array[count].name, name);
			count++;
		}
		fclose(file);
	}
	// ���ο� ��ŷ ���� �߰� (nickname, score)
	if (_tcslen(nickname) > 0 && score > 0) {
		_tcscpy_s(rank_array[count].name, nickname);
		rank_array[count].score = score;
		count++;
	}

	// ������ �������� �������� ����
	for (int i = 0; i < count - 1; i++) {
		for (int j = i + 1; j < count; j++) {
			if (rank_array[i].score < rank_array[j].score) {
				struct Rank temp = rank_array[i];
				rank_array[i] = rank_array[j];
				rank_array[j] = temp;
			}
		}
	}

	// ���ĵ� ��ŷ ������ rank.txt ���Ͽ� �ٽ� ����
	file = _tfopen(_T("rank.txt"), _T("w"));
	if (file) {
		for (int i = 0; i < count; i++) {
			_ftprintf(file, _T("%d. %s %d\n"), i + 1, rank_array[i].name, rank_array[i].score);
		}
		fclose(file);
	}
}

// �̵� ���� ������ �׸���
void DrawMovingBlocks(HDC hdc, int before_board[4][4], int after_board[4][4], int move[4][4][2], int count)
{
	// ���� ������ ��ü�� ��� ������� ���´�
	RECT rect = { 70, 150, 70 + 400, 150 + 400 };
	HBRUSH hBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
	FillRect(hdc, &rect, hBrush);

	int x, y;

	// ��ϵ��� ĭ ���� �׸���
	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			DrawBitmap(hdc, 70 + 100 * col + 20, 150 + 100 * row + 20, 0);
		}
	}

	// �̵� ���� ��� �׸���
	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			if (before_board[row][col] != 0)
			{
				y = move[row][col][0] * 10 * (count); // count�� (10�ȼ�) X (�̵��ؾ��� ĭ ��) ����
				x = move[row][col][1] * 10 * (count); // count�� (10�ȼ�) X (�̵��ؾ��� ĭ ��) ����
				DrawBitmap(hdc, 70 + 100 * col + 20 + x, 150 + 100 * row + 20 + y, before_board[row][col]);
			}
		}
	}
}

// �������� 2�� 4�� �߰��ϴ� �Լ�
int GenerateBlock(int board[4][4])
{
	int count = 0;  // �� ��� ���� ����
	int array[16][2] = {0}; // �� ��ϵ��� ��ǥ �����ϱ�

	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			if (board[row][col] == 0)  // �� ��� ã����
			{
				array[count][0] = row;
				array[count][1] = col;
				count++;
			}
		}
	}

	if (count == 0)
		return 0;
	int num = rand() % count; // �������� ��ȣ ã��
	int two_or_four = rand() % 2; // �������� 2�� 4 ����

	if (two_or_four == 0)
	{
		board[array[num][0]][array[num][1]] = 1;
	}
	else
	{
		board[array[num][0]][array[num][1]] = 2;
	}
	return 1;
}

// ������ �����ϴ� �Լ�
void CopyGameBoard(int copied_board[4][4], int original_board[4][4])
{
	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			copied_board[row][col] = original_board[row][col];
		}
	}
}

// ������ 2���� ������ ���ϴ� �Լ�
int CompareGameBoard(int board1[4][4], int board2[4][4])
{
	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			if (board1[row][col] != board2[row][col]) // �ϳ��� �ٸ��� 0 ��ȯ
				return 0;
		}
	}
	return 1; // ��� ������ 1 ��ȯ
}

// ���� ����Ű ������ ��
void MoveLeft(int board[4][4], int move[4][4][2])
{
	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			if (board[row][col] != 0) {
				for (int target = col - 1; target >= 0; target--) // �ڽ��� ������ Ȯ���Ѵ�
				{
					if (board[row][target] == 0) // ��ĭ�� �߰��ϸ�
					{
						board[row][target] = board[row][target + 1]; // ��ĭ���� ��ĭ �̵�
						board[row][target + 1] = 0; // �̵� �� ���� �ڸ��� ��ĭ����
						move[row][col][1] -= 1; // ���� �̵� ���
					}
					if (board[row][target] == board[row][target + 1]) // ���� ����� �߰��ϸ�
					{
						board[row][target]++; // ����� ��ģ�� (���� ���� ����)
						score += (int)pow(2, board[row][target]); // ��� �������Ƿ� ���� �߰�
						board[row][target + 1] = 0; // �̵� �� ���� �ڸ��� ��ĭ����
						move[row][col][1] -= 1; // ���� �̵� ���
					}
				}
			}
		}
	}
}

// ������ ����Ű ������ ��
void MoveRight(int board[4][4], int move[4][4][2])
{
	for (int row = 0; row < 4; row++)
	{
		for (int col = 3; col >= 0; col--)
		{
			if (board[row][col] != 0) {
				for (int target = col + 1; target < 4; target++) // �ڽ��� �������� Ȯ���Ѵ�
				{
					if (board[row][target] == 0) // ��ĭ�� �߰��ϸ�
					{
						board[row][target] = board[row][target - 1]; // ��ĭ���� ��ĭ �̵�
						board[row][target - 1] = 0; // �̵� �� ���� �ڸ��� ��ĭ����
						move[row][col][1] += 1; // ������ �̵� ���
					}
					if (board[row][target] == board[row][target - 1]) // ���� ����� �߰��ϸ�
					{
						board[row][target]++; // ����� ��ģ�� (���� ���� ����)
						score += (int)pow(2, board[row][target]); // ��� �������Ƿ� ���� �߰�
						board[row][target - 1] = 0; // �̵� �� ���� �ڸ��� ��ĭ����
						move[row][col][1] += 1; // ������ �̵� ���
					}
				}
			}
		}
	}
}

// ���� ����Ű ������ ��
void MoveUp(int board[4][4], int move[4][4][2])
{
	for (int col = 0; col < 4; col++)
	{
		for (int row = 0; row < 4; row++)
		{
			if (board[row][col] != 0) {
				for (int target = row - 1; target >= 0; target--) // �ڽ��� ������ Ȯ���Ѵ�
				{
					if (board[target][col] == 0) // ��ĭ�� �߰��ϸ�
					{
						board[target][col] = board[target + 1][col]; // ��ĭ���� ��ĭ �̵�
						board[target + 1][col] = 0; // �̵� �� ���� �ڸ��� ��ĭ����
						move[row][col][0] -= 1; // ���� �̵� ���
					}
					if (board[target][col] == board[target + 1][col]) // ���� ����� �߰��ϸ�
					{
						board[target][col]++; // ����� ��ģ�� (���� ���� ����)
						score += (int)pow(2, board[target][col]); // ��� �������Ƿ� ���� �߰�
						board[target + 1][col] = 0; // �̵� �� ���� �ڸ��� ��ĭ����
						move[row][col][0] -= 1; // ���� �̵� ���
					}
				}
			}
		}
	}
}

// �Ʒ��� ����Ű ������ ��
void MoveDown(int board[4][4], int move[4][4][2])
{
	for (int col = 0; col < 4; col++)
	{
		for (int row = 3; row >= 0; row--)
		{
			if (board[row][col] != 0) {
				for (int target = row + 1; target < 4; target++) // �ڽ��� �Ʒ����� Ȯ���Ѵ�
				{
					if (board[target][col] == 0) // ��ĭ�� �߰��ϸ�
					{
						board[target][col] = board[target - 1][col]; // ��ĭ���� ��ĭ �̵�
						board[target - 1][col] = 0; // �̵� �� ���� �ڸ��� ��ĭ����
						move[row][col][0] += 1; // �Ʒ��� �̵� ���
					}
					if (board[target][col] == board[target - 1][col]) // ���� ����� �߰��ϸ�
					{
						board[target][col]++; // ����� ��ģ�� (���� ���� ����)
						score += (int)pow(2, board[target][col]); // ��� �������Ƿ� ���� �߰�
						board[target - 1][col] = 0; // �̵� �� ���� �ڸ��� ��ĭ����
						move[row][col][0] += 1; // �Ʒ��� �̵� ���
					}
				}
			}
		}
	}
}
// �׸� �׸��� �Լ�
void DrawBitmap(HDC hdc, int x, int y, int value) {
	if (value == 0) // �� ����̸� �� ���� �׸���
		Rectangle(hdc, x, y, x + 80, y + 80);

	HDC MemDC;
	HBITMAP OldBitmap;

	MemDC = CreateCompatibleDC(hdc);
	OldBitmap = (HBITMAP)SelectObject(MemDC, hBit[value - 1]);

	BitBlt(hdc, x, y, 80, 80, MemDC, 0, 0, SRCCOPY);

	SelectObject(MemDC, OldBitmap);
	DeleteDC(MemDC);
}

// ��ϵ� �׸���
void DrawBlocks(HDC hdc, int x, int y, int board[4][4])
{
	for (int row = 0; row < 4; row++)
	{
		for (int col = 0; col < 4; col++)
		{
			DrawBitmap(hdc, x + 100 * col + 20, y + 100 * row + 20, board[row][col]);
		}
	}
}