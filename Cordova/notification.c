// Copyright 2012 Intel Corporation
//
// Licensed to the Apache Software Foundation (ASF) under one
// or more contributor license agreements.  See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.  The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.  You may obtain a copy of the License at
// 
//    http://www.apache.org/licenses/LICENSE-2.0
// 
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.  See the License for the
// specific language governing permissions and limitations
// under the License.

#include <stdlib.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>

#include "notification.h"

extern HWND	hWnd;

#define FONT_SIZE	10
#define FONT_NAME	L"Arial"
#define MAX_BUTTONS	10
#define ID_BASE		100

LRESULT CALLBACK NotificationDialogProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	int btn_id;
	
	switch (uMsg)
	{
		case WM_INITDIALOG:
			return TRUE;

		case WM_COMMAND:
			if (wParam == IDCANCEL)
			{
				EndDialog(hDlg, -1);
				return FALSE;
			}
			else
			{
				btn_id = (SHORT) (LOWORD(wParam));
				EndDialog(hDlg, btn_id - ID_BASE);	// Use large button IDs to avoid collisions with IDOK, IDCANCEL and friends 
				return TRUE;
			}
	
		default:
			return FALSE;
	}
}


// Align an USHORT pointer to a 4 bytes aligned boundary, padding with zero if necessary
#define ALIGN4(cursor)	if (((BYTE) cursor) & 2) *cursor++ = 0;
   
// See http://msdn.microsoft.com/en-us/library/ms645394%28v=vs.85%29.aspx


LRESULT DisplayMessage(wchar_t* title, int title_len, wchar_t* message, int message_len, wchar_t* button_label[], int button_len[], int num_buttons)
{
    DLGTEMPLATE* dlg_template;
    DLGITEMTEMPLATE* item_template;
    WORD* cursor;	// 16 bits words pointer
    LRESULT ret_code;
	void* buf;
	int i;
	int next_x;
	int button_width = 80;	// Width of a button
	int button_gap = 6;	// Width of the space separating two buttons
	int left_margin = 10;	// Left dialog margin
	int right_margin = 10;	// Right dialog margin
	int top_margin = 10;
	int bottom_margin = 10;
	int static_height = 40;	// Height of the space where static text is displayed
	int static_to_buttons_margin = num_buttons > 0 ? 5 : 0;
	int button_height = num_buttons > 0 ? 15 : 0;
	int num_gaps = num_buttons ? num_buttons -1 : 0;
	int static_width = num_buttons ? num_buttons * button_width + button_gap * num_gaps : 80;
	int buf_len;
	int font_len = wcslen(FONT_NAME);

	// Compute length of work buffer and allocate it
	buf_len = sizeof(DLGTEMPLATE) + 4 + title_len + 1 + font_len + 1 + message_len + 1 + sizeof(DLGITEMTEMPLATE) + 4 + 2 + num_buttons * sizeof(DLGITEMTEMPLATE) + 
				+ 100; // Allow for into account possible alignment padding as well as extra fields (class atoms, user data)

	for (i=0; i<num_buttons; i++)
		buf_len += button_len[i] + 1;	

	buf = malloc(buf_len);

    dlg_template = (DLGTEMPLATE*) buf;
 
    // Dialog header
 
    dlg_template->style = WS_POPUP | WS_BORDER | WS_SYSMENU | DS_MODALFRAME | WS_CAPTION | DS_SETFONT;
	dlg_template->dwExtendedStyle = 0;
    dlg_template->cdit = 1 + num_buttons;         // Number of controls
    dlg_template->x  = 0;			// In Dialog Box Units
	dlg_template->y  = 0;	
    dlg_template->cx = left_margin + static_width + right_margin;
	dlg_template->cy = top_margin + static_height + static_to_buttons_margin + button_height + bottom_margin;

    cursor = (WORD*)(dlg_template + 1);	// Point past DLGTEMPLATE structure
    *cursor++ = 0;            // Menu
    *cursor++ = 0;            // Default Dialog class

    // Copy title, add NUL and shift cursor
	wmemcpy(cursor, title, title_len);
	cursor += title_len;
	*cursor++ = 0;

	// Type point and font name (as DS_FONT was specified)
	*cursor++ = FONT_SIZE;
	wmemcpy(cursor, FONT_NAME, font_len);
	cursor += font_len;
	*cursor++ = 0;

	// Item templates need to be DWORD aligned
	ALIGN4(cursor);

	// Static control

    item_template = (DLGITEMTEMPLATE*) cursor;
    item_template->style = WS_CHILD | WS_VISIBLE | SS_CENTER;
	item_template->dwExtendedStyle = 0;
	item_template->x  = left_margin;
	item_template->y  = top_margin;
    item_template->cx = static_width;
	item_template->cy = static_height;
    item_template->id = -1;

    // Move past DLGITEMTEMPLATE structure
	cursor = (WORD*)(item_template + 1);
  
	// Static class
	*cursor++ = 0xFFFF;
    *cursor++ = 0x0082;

	// Title
	wmemcpy(cursor, message, message_len);
	cursor += message_len;
	*cursor++ = 0;

    // Empty user data block
	*cursor++ = 0;

	next_x = left_margin;
	
	// Additional controls
	for (i=0; i<num_buttons; i++)
	{
		ALIGN4(cursor);

		item_template = (DLGITEMTEMPLATE*) cursor;
		item_template->style = WS_CHILD | WS_VISIBLE;
		item_template->dwExtendedStyle = 0;
		item_template->x  = next_x;
		item_template->y  = top_margin + static_height + static_to_buttons_margin;
		item_template->cx = button_width;
		item_template->cy = button_height;
		item_template->id = ID_BASE + i;

		next_x += button_width + button_gap;

		// Move past DLGITEMTEMPLATE structure
		cursor = (WORD*)(item_template + 1);
   
		// Button class
		*cursor++ = 0xFFFF;
		*cursor++ = 0x0080;

		// Title
		wmemcpy(cursor, button_label[i], button_len[i]);
		cursor += button_len[i];
		*cursor++ = 0; 
  
		// Empty user data block
		*cursor++ = 0;             
	}

	ret_code = DialogBoxIndirect(GetModuleHandle(0), dlg_template, hWnd, NotificationDialogProc); 
    free(buf); 
    return ret_code; 
}

static HRESULT show_dialog(BSTR callback_id, BSTR args)
{
	wchar_t buf[10];
	int ret_code;
	wchar_t* message = 0;
	int message_len = 0;
	wchar_t* title = 0;
	int title_len = 0;
	int num_buttons = 0;
	wchar_t* btn_text[MAX_BUTTONS];
	int btn_text_len[MAX_BUTTONS];
	wchar_t separator;

	wchar_t* cursor = args;

	// args should be like "["message", "title", "button1, button2"]"

	// Search for first quote
	while (*cursor && *cursor != L'\'' && *cursor != L'"')
		cursor++;

	// Record whether it's a single or double quote
	separator = *cursor;

	if (!separator)
		return -1;

	// Mark beginning of message
	cursor++;
	message = cursor;

	// Search for next quote
	while (*cursor && *cursor != separator)
		cursor++;

	if (*cursor != separator)
		return -1;

	// Found it ; record message len
	message_len = cursor - message;

	// Move past second quote
	cursor++;

	// Done with message, now look for title

	while (*cursor && *cursor != L'\'' && *cursor != L'"')
		cursor++;

	separator = *cursor;

	if (!separator)
		return -1;

	cursor++;
	title = cursor;

	while (*cursor && *cursor != separator)
		cursor++;

	if (*cursor != separator)
		return -1;

	title_len = cursor - title;

	cursor++;

	// Done with title, switch to buttons
	
	// Locate next quote
	while (*cursor && *cursor != L'\'' && *cursor != L'"')
		cursor++;

	if (!*cursor)
		goto button_done; // No button ; consider that a valid use case

	cursor++;

button_parsing:

	// Skip spaces
	while (*cursor && *cursor == L' ')
		cursor++;

	if (*cursor)
	{
		btn_text[num_buttons] = cursor;

		cursor++;

		// Search for end marker
		while (*cursor && *cursor != L',' && *cursor != L'"')
			cursor++;

		btn_text_len[num_buttons] = cursor - btn_text[num_buttons];

		num_buttons++;

		cursor++;
	
		if (*cursor != 0 && *cursor != L'"' && *cursor != L'\'' && *cursor != L']' && num_buttons < MAX_BUTTONS)
			goto button_parsing;
	}

button_done:

	ret_code = DisplayMessage(title, title_len, message, message_len, btn_text, btn_text_len, num_buttons);

	wsprintf(buf, L"%d", ret_code);

	cordova_success_callback(callback_id, FALSE, buf);

	return S_OK;
}

static HRESULT vibrate(BSTR callback_id, BSTR args)
{
	return S_OK;
}

static HRESULT beep(BSTR callback_id, BSTR args)
{
	int count;

	args++; // skip initial '['
	*(args + wcslen(args) - 1) = 0; // remove trailing ']'

	for (count = _wtoi(args); count > 0; count--) {
		MessageBeep(0xFFFFFFFF);
		Sleep(100);
	}

	return S_OK;
}

HRESULT notification_exec(BSTR callback_id, BSTR action, BSTR args, VARIANT *result)
{
	if(!wcscmp(action, L"alert") || !wcscmp(action, L"confirm"))
		return show_dialog(callback_id, args);
	if (!wcscmp(action, L"vibrate"))
		return vibrate(callback_id, args);
	if (!wcscmp(action, L"beep"))
		return beep(callback_id, args);

	return DISP_E_MEMBERNOTFOUND;
}

DEFINE_CORDOVA_MODULE(Notification, L"Notification", notification_exec, NULL)