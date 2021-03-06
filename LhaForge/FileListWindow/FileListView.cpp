/*
 * Copyright (c) 2005-2012, Claybird
 * All rights reserved.

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:

 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the Claybird nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.

 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 */

#include "stdafx.h"
#include "FileListView.h"
#include "../resource.h"
#include "../Dialogs/LogDialog.h"
#include "../Dialogs/LFFolderDialog.h"
#include "../ConfigCode/ConfigFileListWindow.h"
#include "../Utilities/StringUtil.h"
#include <sstream>

CFileListView::CFileListView(CConfigManager& rConfig,CFileListModel& rModel):
	mr_Config(rConfig),
	mr_Model(rModel),
	m_bDisplayFileSizeInByte(false),
	//m_DnDSource(m_TempDirManager),
	m_DropTarget(this),
	m_nDropHilight(-1),
	m_hFrameWnd(NULL),
	m_TempDirMgr(_T("lhaf"))
{
}

LRESULT CFileListView::OnCreate(LPCREATESTRUCT lpcs)
{
	LRESULT lRes=DefWindowProc();
	//SetFont(AtlGetDefaultGuiFont());

	// イメージリスト作成
	m_ShellDataManager.Init();
	SetImageList(m_ShellDataManager.GetImageList(true),LVSIL_NORMAL);
	SetImageList(m_ShellDataManager.GetImageList(false),LVSIL_SMALL);

	//カラムヘッダのソートアイコン
	m_SortImageList.CreateFromImage(MAKEINTRESOURCE(IDB_BITMAP_SORTMARK),16,1,CLR_DEFAULT,IMAGE_BITMAP,LR_CREATEDIBSECTION);

	mr_Model.addEventListener(m_hWnd);
	return lRes;
}

LRESULT CFileListView::OnDestroy()
{
	mr_Model.removeEventListener(m_hWnd);
	return 0;
}



bool CFileListView::SetColumnState(const int* pColumnOrderArray)
{
	//既存のカラムを削除
	if(GetHeader().IsWindow()){
		int nCount=GetHeader().GetItemCount();
		for(;nCount>0;nCount--){
			DeleteColumn(nCount-1);
		}
	}

//========================================
//      リストビューにカラム追加
//========================================
//リストビューにカラムを追加するためのマクロ
#define ADD_COLUMNITEM(x,width,pos) \
{if(-1!=UtilCheckNumberArray(pColumnOrderArray,FILEINFO_ITEM_COUNT,FILEINFO_##x)){\
	int nIndex=InsertColumn(FILEINFO_##x, CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_##x)), pos, width,-1);\
	if(nIndex<0||nIndex>=FILEINFO_ITEM_COUNT)return false;\
	m_ColumnIndexArray[nIndex]=FILEINFO_##x;\
}}

	memset(m_ColumnIndexArray,-1,sizeof(m_ColumnIndexArray));

	//ファイル名
	ADD_COLUMNITEM(FILENAME,100,LVCFMT_LEFT);
	//フルパス名
	ADD_COLUMNITEM(FULLPATH,200,LVCFMT_LEFT);
	//ファイルサイズ
	ADD_COLUMNITEM(ORIGINALSIZE,90,LVCFMT_LEFT);
	//ファイル種類
	ADD_COLUMNITEM(TYPENAME,120,LVCFMT_LEFT);
	//更新日時
	ADD_COLUMNITEM(FILETIME,120,LVCFMT_LEFT);
	//属性
	ADD_COLUMNITEM(ATTRIBUTE,60,LVCFMT_LEFT);
	//圧縮後サイズ
	ADD_COLUMNITEM(COMPRESSEDSIZE,90,LVCFMT_RIGHT);
	//圧縮メソッド
	ADD_COLUMNITEM(METHOD,60,LVCFMT_LEFT);
	//圧縮率
	ADD_COLUMNITEM(RATIO,60,LVCFMT_RIGHT);
	//CRC
	ADD_COLUMNITEM(CRC,60,LVCFMT_LEFT);


	//----------------------
	// カラムの並び順を設定
	//----------------------
	int Count=0;
	for(;Count<FILEINFO_ITEM_COUNT;Count++){
		//有効なアイテム数を求める
		if(-1==pColumnOrderArray[Count])break;
	}
	//並び順を変換
	int TemporaryArray[FILEINFO_ITEM_COUNT];
	//memcpy(TemporaryArray,pColumnOrderArray,sizeof(TemporaryArray));
	for(int i=0;i<FILEINFO_ITEM_COUNT;i++){
		TemporaryArray[i]=pColumnOrderArray[i];
	}
	for(int i=0;i<Count;i++){
		int nIndex=UtilCheckNumberArray(m_ColumnIndexArray,FILEINFO_ITEM_COUNT,TemporaryArray[i]);
		ASSERT(-1!=nIndex);
		if(-1!=nIndex){
			TemporaryArray[i]=nIndex;
		}
	}
	SetColumnOrderArray(Count,TemporaryArray);

	//カラムヘッダのソートアイコン
	if(GetHeader().IsWindow()){
		GetHeader().SetImageList(m_SortImageList);
	}
	UpdateSortIcon();
	return true;
}

void CFileListView::GetColumnState(int* pColumnOrderArray)
{
	//カラムの並び順取得
	const int nCount=GetHeader().GetItemCount();
	ASSERT(nCount<=FILEINFO_ITEM_COUNT);

	int TemporaryArray[FILEINFO_ITEM_COUNT];
	memset(TemporaryArray,-1,sizeof(TemporaryArray));
	GetColumnOrderArray(nCount,TemporaryArray);
	//並び順を変換
	memset(pColumnOrderArray,-1,FILEINFO_ITEM_COUNT*sizeof(int));
	for(int i=0;i<nCount;i++){
		pColumnOrderArray[i]=m_ColumnIndexArray[TemporaryArray[i]];
	}
}

void CFileListView::GetSelectedItems(std::list<ARCHIVE_ENTRY_INFO_TREE*> &items)
{
	items.clear();
	int nIndex=-1;
	for(;;){
		nIndex = GetNextItem(nIndex, LVNI_ALL | LVNI_SELECTED);
		if(-1==nIndex)break;
		ARCHIVE_ENTRY_INFO_TREE* lpNode=mr_Model.GetFileListItemByIndex(nIndex);

		ASSERT(lpNode);

		items.push_back(lpNode);
	}
}

LRESULT CFileListView::OnFileListNewContent(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE(__FUNCTIONW__ _T("\n"));

	DeleteAllItems();
	SetItemCount(0);
	if(mr_Model.IsOK()){
		ARCHIVE_ENTRY_INFO_TREE* lpCurrent=mr_Model.GetCurrentNode();
		ASSERT(lpCurrent);
		if(lpCurrent){
			SetItemCount(lpCurrent->GetNumChildren());
			SetItemState(0,LVIS_FOCUSED,LVIS_FOCUSED);
		}
	}

	//ファイルドロップの受け入れ:受け入れ拒否はDragOverが行う
	EnableDropTarget(true);
	return 0;
}

LRESULT CFileListView::OnFileListUpdated(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	TRACE(__FUNCTIONW__ _T("\n"));

	Invalidate();

	return 0;
}

LRESULT CFileListView::OnDblClick(LPNMHDR pnmh)
{
	TRACE(__FUNCTIONW__ _T("\n"));
	//----------------------------------------------------------------------
	//選択アイテムが複数の時:
	// 選択を関連付けで開く
	//Shiftが押されていた時:
	// 選択を関連付けで開く
	//選択アイテムが一つだけの時:
	// フォルダをダブルクリックした/Enterを押した場合にはそのフォルダを開く
	// 選択がファイルだった場合には関連付けで開く
	//----------------------------------------------------------------------

	//選択アイテムが複数の時
	//もしShiftが押されていたら、関連付けで開く
	if(GetKeyState(VK_SHIFT)<0||GetSelectedCount()>=2){
		OpenAssociation(false,true);
		return 0;
	}

	//選択されたアイテムを取得
	std::list<ARCHIVE_ENTRY_INFO_TREE*> items;
	GetSelectedItems(items);
	if(items.empty())return 0;
	ARCHIVE_ENTRY_INFO_TREE* lpNode=*(items.begin());

	if(lpNode->bDir){
		//階層構造を無視する場合には、この動作は無視される
		if(mr_Model.GetListMode()==FILELIST_TREE){
			mr_Model.MoveDownDir(lpNode);
		}
	}else{
		OpenAssociation(false,true);
	}
	return 0;
}


//カラム表示のOn/Offを切り替える
//表示中:該当カラムを非表示にし、配列を詰める
//非表示:使われていない部分に指定カラムを追加
void _ToggleColumn(int *lpArray,size_t size,FILEINFO_TYPE type)
{
	ASSERT(lpArray);
	if(!lpArray)return;

	for(size_t i=0;i<size;i++){
		if(type==lpArray[i]){
			//配列を詰める
			for(size_t j=i;j<size-1;j++){
				lpArray[j]=lpArray[j+1];
			}
			lpArray[size-1]=-1;
			return;
		}
		else if(-1==lpArray[i]){
			lpArray[i]=type;
			return;
		}
	}
}


//カラムヘッダを左/右クリック
LRESULT CFileListView::OnColumnRClick(int /*idCtrl*/, LPNMHDR pnmh, BOOL& bHandled)
{
	if(pnmh->hwndFrom!=GetHeader()){
		//メッセージは処理しなかったことにする
		bHandled = FALSE;
		return 0;
	}

	//右クリックメニュー表示
	POINT point;
	GetCursorPos(&point);
	CMenu cMenu;
	cMenu.LoadMenu(IDR_LISTVIEW_HEADER_MENU);
	CMenuHandle cSubMenu(cMenu.GetSubMenu(0));

	//--------------------------------
	// 各メニューアイテムの有効・無効
	//--------------------------------

	int columnOrderArray[FILEINFO_ITEM_COUNT];
	GetColumnState(columnOrderArray);

	struct{
		FILEINFO_TYPE idx;
		UINT nMenuID;
	}menuTable[]={
		{FILEINFO_FULLPATH,			ID_MENUITEM_LISTVIEW_COLUMN_FULLPATH},
		{FILEINFO_ORIGINALSIZE,		ID_MENUITEM_LISTVIEW_COLUMN_ORIGINALSIZE},
		{FILEINFO_TYPENAME,			ID_MENUITEM_LISTVIEW_COLUMN_TYPENAME},
		{FILEINFO_FILETIME,			ID_MENUITEM_LISTVIEW_COLUMN_FILETIME},
		{FILEINFO_ATTRIBUTE,		ID_MENUITEM_LISTVIEW_COLUMN_ATTRIBUTE},
		{FILEINFO_COMPRESSEDSIZE,	ID_MENUITEM_LISTVIEW_COLUMN_COMPRESSEDSIZE},
		{FILEINFO_METHOD,			ID_MENUITEM_LISTVIEW_COLUMN_METHOD},
		{FILEINFO_RATIO,			ID_MENUITEM_LISTVIEW_COLUMN_RATIO},
		{FILEINFO_CRC,				ID_MENUITEM_LISTVIEW_COLUMN_CRC},
	};

	for(size_t i=0;i<COUNTOF(menuTable);i++){
		bool bEnabled=(-1!=UtilCheckNumberArray(columnOrderArray,COUNTOF(columnOrderArray),menuTable[i].idx));
		cSubMenu.CheckMenuItem(menuTable[i].nMenuID,MF_BYCOMMAND|(bEnabled?MF_CHECKED:MF_UNCHECKED));
	}

	//メニュー表示:選択したコマンドが返ってくる
	int nRet=cSubMenu.TrackPopupMenu(TPM_NONOTIFY|TPM_RETURNCMD|TPM_LEFTALIGN|TPM_RIGHTBUTTON,point.x, point.y, m_hWnd,NULL);
	if(0==nRet){
		//Not Selected
		return 0;
	}else if(ID_MENUITEM_LISTVIEW_COLUMN_RESET==nRet){
		//初期化
		for(size_t i=0;i<COUNTOF(columnOrderArray);i++){
			columnOrderArray[i]=i;
		}
	}else{
		for(size_t i=0;i<COUNTOF(menuTable);i++){
			if(menuTable[i].nMenuID==nRet){
				_ToggleColumn(columnOrderArray,COUNTOF(columnOrderArray),menuTable[i].idx);
			}
		}
	}

	SetColumnState(columnOrderArray);

	return 0;
}


//キー入力によるファイル検索
LRESULT CFileListView::OnFindItem(LPNMHDR pnmh)
{
	ARCHIVE_ENTRY_INFO_TREE* lpCurrent=mr_Model.GetCurrentNode();
	ASSERT(lpCurrent);
	if(!lpCurrent)return -1;

	int iCount=lpCurrent->GetNumChildren();
	if(iCount<=0)return -1;

	LPNMLVFINDITEM lpFindInfo = (LPNMLVFINDITEM)pnmh;
	int iStart=lpFindInfo->iStart;
	if(iStart<0)iStart=0;
	if(lpFindInfo->lvfi.flags & LVFI_STRING || lpFindInfo->lvfi.flags & LVFI_PARTIAL){	//ファイル名で検索
		LPCTSTR lpFindString=lpFindInfo->lvfi.psz;
		size_t nLength=_tcslen(lpFindString);
		//前方一致で検索
		for(int i=iStart;i<iCount;i++){
			ARCHIVE_ENTRY_INFO_TREE* lpNode=mr_Model.GetFileListItemByIndex(i);
			ASSERT(lpNode);
			if(0==_tcsnicmp(lpFindString,lpNode->strTitle,nLength)){
				return i;
			}
		}
		if(lpFindInfo->lvfi.flags & LVFI_WRAP){
			for(int i=0;i<iStart;i++){
				ARCHIVE_ENTRY_INFO_TREE* lpNode=mr_Model.GetFileListItemByIndex(i);
				ASSERT(lpNode);
				if(0==_tcsnicmp(lpFindString,lpNode->strTitle,nLength)){
					return i;
				}
			}
		}
		return -1;
	}else{
		return -1;
	}
}



//ソート
LRESULT CFileListView::OnSortItem(LPNMHDR pnmh)
{
	LPNMLISTVIEW lpNMLV=(LPNMLISTVIEW)pnmh;
	int iCol=lpNMLV->iSubItem;
	SortItem(iCol);
	return 0;
}


void CFileListView::SortItem(int iCol)
{
	if(!(iCol >= 0 && iCol < FILEINFO_ITEM_COUNT))return;

	if(iCol==mr_Model.GetSortKeyType()){
		if(mr_Model.GetSortMode()){
			mr_Model.SetSortMode(false);
		}else{	//ソート解除
			mr_Model.SetSortKeyType(FILEINFO_INVALID);
			mr_Model.SetSortMode(true);
		}
	}else{
		mr_Model.SetSortKeyType(iCol);
		mr_Model.SetSortMode(true);
	}

	UpdateSortIcon();
}

void CFileListView::UpdateSortIcon()
{
	if(!IsWindow())return;
	//ソート状態を元にカラムを変更する
	CHeaderCtrl hc=GetHeader();
	ASSERT(hc.IsWindow());
	if(hc.IsWindow()){
		int count=hc.GetItemCount();
		//アイコン解除
		for(int i=0;i<count;i++){
			HDITEM hdi={0};
			hdi.mask=HDI_FORMAT;
			hc.GetItem(i,&hdi);
			if((hdi.fmt & HDF_IMAGE)){
				hdi.fmt&=~HDF_IMAGE;
				hc.SetItem(i,&hdi);
			}
		}

		//アイコンセット
		int iCol=mr_Model.GetSortKeyType();
		if(iCol!=FILEINFO_INVALID && iCol<count){
			HDITEM hdi={0};
			hdi.mask=HDI_FORMAT;

			hc.GetItem(iCol,&hdi);
			hdi.mask|=HDI_FORMAT|HDI_IMAGE;
			hdi.fmt|=HDF_IMAGE|HDF_BITMAP_ON_RIGHT;
			hdi.iImage=mr_Model.GetSortMode() ? 0 : 1;
			hc.SetItem(iCol,&hdi);
		}
	}
}

//カスタムドロー
DWORD CFileListView::OnPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd)
{
	if(lpnmcd->hdr.hwndFrom == m_hWnd)
		return CDRF_NOTIFYITEMDRAW;
	else
		return CDRF_DODEFAULT;
}

DWORD CFileListView::OnItemPrePaint(int nID, LPNMCUSTOMDRAW lpnmcd)
{
	if(lpnmcd->hdr.hwndFrom == m_hWnd){
		LPNMLVCUSTOMDRAW lpnmlv = (LPNMLVCUSTOMDRAW)lpnmcd;

		ARCHIVE_ENTRY_INFO_TREE* lpNode=mr_Model.GetFileListItemByIndex(lpnmcd->dwItemSpec);
		if(lpNode){
			if(!lpNode->bSafe){
				//危険なアーカイブなので色を付ける
				lpnmlv->clrText = RGB(255, 255, 255);
				lpnmlv->clrTextBk = RGB(255, 0, 0);
			}
		}
	}
	return CDRF_DODEFAULT;
}


//仮想リストビューのアイテム取得に反応
LRESULT CFileListView::OnGetDispInfo(LPNMHDR pnmh)
{
	LV_DISPINFO* pstLVDInfo=(LV_DISPINFO*)pnmh;

	ARCHIVE_ENTRY_INFO_TREE* lpNode=mr_Model.GetFileListItemByIndex(pstLVDInfo->item.iItem);
	//ASSERT(lpNode);
	if(!lpNode)return 0;

	//添え字チェック
	ASSERT(pstLVDInfo->item.iSubItem>=0 && pstLVDInfo->item.iSubItem<FILEINFO_ITEM_COUNT);
	if(pstLVDInfo->item.iSubItem<0||pstLVDInfo->item.iSubItem>=FILEINFO_ITEM_COUNT)return 0;

	CString strBuffer;
	LPCTSTR lpText=NULL;
	switch(m_ColumnIndexArray[pstLVDInfo->item.iSubItem]){
	case FILEINFO_FILENAME:	//ファイル名
		if(pstLVDInfo->item.mask & LVIF_TEXT)lpText=lpNode->strTitle;
		if(pstLVDInfo->item.mask & LVIF_IMAGE)pstLVDInfo->item.iImage=m_ShellDataManager.GetIconIndex(lpNode->strExt);
		break;
	case FILEINFO_FULLPATH:	//格納パス
		if(pstLVDInfo->item.mask & LVIF_TEXT)lpText=lpNode->strFullPath;
		break;
	case FILEINFO_ORIGINALSIZE:	//サイズ(圧縮前)
		if(pstLVDInfo->item.mask & LVIF_TEXT){
			FormatFileSize(strBuffer,lpNode->llOriginalSize);
			lpText=strBuffer;
		}
		break;
	case FILEINFO_COMPRESSEDSIZE:	//サイズ(圧縮後)
		if(pstLVDInfo->item.mask & LVIF_TEXT){
			FormatFileSize(strBuffer,lpNode->llCompressedSize);
			lpText=strBuffer;
		}
		break;
	case FILEINFO_TYPENAME:	//ファイルタイプ
		if(pstLVDInfo->item.mask & LVIF_TEXT)lpText=m_ShellDataManager.GetTypeName(lpNode->strExt);
		break;
	case FILEINFO_FILETIME:	//ファイル日時
		if(pstLVDInfo->item.mask & LVIF_TEXT){
			FormatFileTime(strBuffer,lpNode->cFileTime);
			lpText=strBuffer;
		}
		break;
	case FILEINFO_ATTRIBUTE:	//属性
		if(pstLVDInfo->item.mask & LVIF_TEXT){
			FormatAttribute(strBuffer,lpNode->nAttribute);
			lpText=strBuffer;
		}
		break;
	case FILEINFO_METHOD:	//圧縮メソッド
		if(pstLVDInfo->item.mask & LVIF_TEXT)lpText=lpNode->strMethod;
		break;
	case FILEINFO_RATIO:	//圧縮率
		if(pstLVDInfo->item.mask & LVIF_TEXT){
			FormatRatio(strBuffer,lpNode->wRatio);
			lpText=strBuffer;
		}
		break;
	case FILEINFO_CRC:	//CRC
		if(pstLVDInfo->item.mask & LVIF_TEXT){
			FormatCRC(strBuffer,lpNode->dwCRC);
			lpText=strBuffer;
		}
		break;
	}

	if(lpText){
		_tcsncpy_s(pstLVDInfo->item.pszText,pstLVDInfo->item.cchTextMax, lpText,pstLVDInfo->item.cchTextMax);
	}
	return 0;
}

//仮想リストビューのツールチップ取得に反応
LRESULT CFileListView::OnGetInfoTip(LPNMHDR pnmh)
{
	LPNMLVGETINFOTIP pGetInfoTip=(LPNMLVGETINFOTIP)pnmh;

	ARCHIVE_ENTRY_INFO_TREE* lpNode=mr_Model.GetFileListItemByIndex(pGetInfoTip->iItem);
	ASSERT(lpNode);
	if(!lpNode)return 0;
	CString strInfo;
	CString strBuffer;

	//ファイル名
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_FILENAME));
	strInfo+=_T(" : ");	strInfo+=lpNode->strTitle;		strInfo+=_T("\n");
	//格納パス
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_FULLPATH));
	strInfo+=_T(" : ");	strInfo+=lpNode->strFullPath;	strInfo+=_T("\n");
	//圧縮前サイズ
	FormatFileSize(strBuffer,lpNode->llOriginalSize);
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_ORIGINALSIZE));
	strInfo+=_T(" : ");	strInfo+=strBuffer;		strInfo+=_T("\n");
	//ファイルタイプ
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_TYPENAME));
	strInfo+=_T(" : ");	strInfo+=m_ShellDataManager.GetTypeName(lpNode->strExt);	strInfo+=_T("\n");
	//ファイル日時
	FormatFileTime(strBuffer,lpNode->cFileTime);
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_FILETIME));
	strInfo+=_T(" : ");	strInfo+=strBuffer;		strInfo+=_T("\n");
	//属性
	FormatAttribute(strBuffer,lpNode->nAttribute);
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_ATTRIBUTE));
	strInfo+=_T(" : ");	strInfo+=strBuffer;		strInfo+=_T("\n");
	//圧縮後サイズ
	FormatFileSize(strBuffer,lpNode->llCompressedSize);
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_COMPRESSEDSIZE));
	strInfo+=_T(" : ");	strInfo+=strBuffer;		strInfo+=_T("\n");
	//圧縮メソッド
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_METHOD));
	strInfo+=_T(" : ");	strInfo+=lpNode->strMethod;	strInfo+=_T("\n");
	//圧縮率
	FormatRatio(strBuffer,lpNode->wRatio);
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_RATIO));
	strInfo+=_T(" : ");	strInfo+=strBuffer;		strInfo+=_T("\n");
	//CRC
	FormatCRC(strBuffer,lpNode->dwCRC);
	strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_COLUMN_CRC));
	strInfo+=_T(" : ");	strInfo+=strBuffer;		strInfo+=_T("\n");

	if(lpNode->nAttribute&FA_DIREC){
		//--------------
		// ディレクトリ
		//--------------
		//子ノードの数を表示
		strInfo+=_T("\n");
		strInfo+=CString(MAKEINTRESOURCE(IDS_FILELIST_SUBITEM));
		strInfo+=_T(" : ");
		strInfo.AppendFormat(IDS_FILELIST_ITEMCOUNT,lpNode->GetNumChildren());
	}

	//InfoTipに設定
	_tcsncpy_s(pGetInfoTip->pszText,pGetInfoTip->cchTextMax,strInfo,pGetInfoTip->cchTextMax);
	return 0;
}

void CFileListView::FormatFileSizeInBytes(CString &Info,const LARGE_INTEGER &_Size)
{
	CString format(MAKEINTRESOURCE(IDS_ORDERUNIT_BYTE));

	std::wstringstream ss;
	ss.imbue(std::locale(""));
	ss << (unsigned __int64)_Size.QuadPart << std::endl;

	Info.Format(format,ss.str().c_str());
}

void CFileListView::FormatFileSize(CString &Info,const LARGE_INTEGER &_Size)
{
	LARGE_INTEGER Size=_Size;
	bool bInByte=m_bDisplayFileSizeInByte;
	Info.Empty();
	if(-1==Size.LowPart&&-1==Size.HighPart){
		Info=_T("---");
		return;
	}

	static CString OrderUnit[]={MAKEINTRESOURCE(IDS_ORDERUNIT_BYTE),MAKEINTRESOURCE(IDS_ORDERUNIT_KILOBYTE),MAKEINTRESOURCE(IDS_ORDERUNIT_MEGABYTE),MAKEINTRESOURCE(IDS_ORDERUNIT_GIGABYTE),MAKEINTRESOURCE(IDS_ORDERUNIT_TERABYTE)};	//サイズの単位
	static const int MAX_ORDERUNIT=COUNTOF(OrderUnit);

	if(bInByte){	//ファイルサイズをバイト単位で表記する
		FormatFileSizeInBytes(Info,_Size);
		return;
	}

	int Order=0;
	for(;Order<MAX_ORDERUNIT;Order++){
		if(Size.QuadPart<1024*1024){
			break;
		}
		Size.QuadPart=Int64ShrlMod32(Size.QuadPart,10);	//1024で割る
	}
	if(0==Order && Size.QuadPart<1024){
		//1KBに満たないのでバイト単位でそのまま表記
		FormatFileSizeInBytes(Info,_Size);
	}else{
		TCHAR Buffer[64]={0};
		if(Order<MAX_ORDERUNIT-1){
			double SizeToDisplay=Size.QuadPart/1024.0;
			Order++;
			Info.Format(OrderUnit[Order],SizeToDisplay);
		}else{
			//過大サイズ
			Info.Format(OrderUnit[Order],Size.QuadPart);
		}
	}
}

void CFileListView::FormatFileTime(CString &Info,const FILETIME &rFileTime)
{
	Info.Empty();
	if(-1==rFileTime.dwHighDateTime && -1==rFileTime.dwLowDateTime){
		Info=_T("------");
	}else{
		FILETIME LocalFileTime;
		SYSTEMTIME SystemTime;

		FileTimeToLocalFileTime(&rFileTime,&LocalFileTime);
		FileTimeToSystemTime(&LocalFileTime, &SystemTime);
		TCHAR Buffer[64];

		wsprintf(Buffer,CString(MAKEINTRESOURCE(IDS_FILELIST_FILETIME_FORMAT)),
				SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay,
				SystemTime.wHour, SystemTime.wMinute,SystemTime.wSecond);
		Info+=Buffer;
	}
}

void CFileListView::FormatAttribute(CString &strBuffer,int nAttribute)
{
	if(nAttribute&FA_UNKNOWN){
		strBuffer=_T("?????");
	}else{
		strBuffer="";
		strBuffer+=(nAttribute&FA_RDONLY) ? _T("R") : _T("-");
		strBuffer+=(nAttribute&FA_HIDDEN) ? _T("H") : _T("-");
		strBuffer+=(nAttribute&FA_SYSTEM) ? _T("S") : _T("-");
		strBuffer+=(nAttribute&FA_DIREC)  ? _T("D") : _T("-");
		strBuffer+=(nAttribute&FA_ARCH)   ? _T("A") : _T("-");
	}
}

void CFileListView::FormatRatio(CString &strBuffer,WORD wRatio)
{
	if(0xFFFF==wRatio)strBuffer=_T("?????");	//取得失敗
	else strBuffer.Format(_T("%.1f%%"),(double)wRatio/10.0);
}

void CFileListView::FormatCRC(CString &strBuffer,DWORD dwCRC)
{
	if(-1==dwCRC)strBuffer=_T("?????");	//取得失敗
	else strBuffer.Format(_T("%08x"),dwCRC);
}

//-------

void CFileListView::OnClearTemporary(UINT uNotifyCode,int nID,HWND hWndCtrl)
{
	mr_Model.ClearTempDir();
}


void CFileListView::OnOpenAssociation(UINT uNotifyCode,int nID,HWND hWndCtrl)
{
	//trueなら存在するテンポラリファイルを削除してから解凍する
	const bool bOverwrite=(nID==ID_MENUITEM_OPEN_ASSOCIATION_OVERWRITE);
	OpenAssociation(bOverwrite,true);
}


void CFileListView::OnExtractTemporary(UINT uNotifyCode,int nID,HWND hWndCtrl)
{
	//上書きはするが、開かない
	OpenAssociation(true,false);
}

//bOverwrite:trueなら存在するテンポラリファイルを削除してから解凍する
bool CFileListView::OpenAssociation(bool bOverwrite,bool bOpen)
{
	if(!mr_Model.IsExtractEachSupported()){
		//選択ファイルの解凍はサポートされていない
		ErrorMessage(CString(MAKEINTRESOURCE(IDS_ERROR_FILELIST_EXTRACT_SELECTED_NOT_SUPPORTED)));
		return false;
	}

	if(!mr_Model.CheckArchiveExists()){	//存在しないならエラー
		CString msg;
		msg.Format(IDS_ERROR_FILE_NOT_FOUND,mr_Model.GetArchiveFileName());
		ErrorMessage(msg);
		return false;
	}

	//選択されたアイテムを列挙
	std::list<ARCHIVE_ENTRY_INFO_TREE*> items;
	GetSelectedItems(items);

	if(!items.empty()){
		std::list<CString> filesList;
		CString strLog;
		if(!mr_Model.MakeSureItemsExtracted(NULL,mr_Model.GetRootNode(),items,filesList,bOverwrite,strLog)){
			CLogDialog LogDialog;
			LogDialog.SetData(strLog);
			LogDialog.DoModal();
		}
		if(bOpen)OpenAssociation(filesList);
	}

	return true;
}

void CFileListView::OpenAssociation(const std::list<CString> &filesList)
{
	for(std::list<CString>::const_iterator ite=filesList.begin();ite!=filesList.end();++ite){
		//拒否されたら上書きも追加解凍もしない;ディレクトリなら拒否のみチェック
		bool bDenyOnly=BOOL2bool(::PathIsDirectory(*ite));//lpNode->bDir;
		if(UtilPathAcceptSpec(*ite,mr_Model.GetOpenAssocExtDeny(),mr_Model.GetOpenAssocExtAccept(),bDenyOnly)){
			::ShellExecute(GetDesktopWindow(),NULL,*ite,NULL,NULL,SW_SHOW);
			TRACE(_T("%s\n"),(LPCTSTR)*ite);
			//::ShellExecute(GetDesktopWindow(),_T("explore"),*ite,NULL,NULL,SW_SHOW);
		}else{
			::MessageBeep(MB_ICONEXCLAMATION);
		}
	}
}

HRESULT CFileListView::AddItems(const std::list<CString> &fileList,LPCTSTR strDest)
{
	//追加開始
	::EnableWindow(m_hFrameWnd,FALSE);
	CString strLog;

	HRESULT hr=mr_Model.AddItem(fileList,strDest,strLog);

	::EnableWindow(m_hFrameWnd,TRUE);
	::SetForegroundWindow(m_hFrameWnd);

	if(FAILED(hr) || S_FALSE==hr){
		CString msg;
		switch(hr){
		case E_LF_SAME_INPUT_AND_OUTPUT:	//アーカイブ自身を追加しようとした
			msg.Format(IDS_ERROR_SAME_INPUT_AND_OUTPUT,mr_Model.GetArchiveFileName());
			ErrorMessage(msg);
			break;
		case E_LF_UNICODE_NOT_SUPPORTED:	//ファイル名にUNICODE文字を持つファイルを圧縮しようとした
			ErrorMessage(CString(MAKEINTRESOURCE(IDS_ERROR_UNICODEPATH)));
			break;
		case S_FALSE:	//追加処理に問題
			{
				CLogDialog LogDialog;
				LogDialog.SetData(strLog);
				LogDialog.DoModal();
			}
			break;
		default:
			ASSERT(!"This code cannot be run");
		}
		return E_INVALIDARG;
	}
	//アーカイブ更新
	::PostMessage(m_hFrameWnd,WM_FILELIST_REFRESH,0,0);
	return S_OK;
}

void CFileListView::OnAddItems(UINT uNotifyCode,int nID,HWND hWndCtrl)
{
	ASSERT(mr_Model.IsAddItemsSupported());
	if(!mr_Model.IsAddItemsSupported())return;

	CString strDest;	//放り込む先
	//カレントフォルダに追加
	ArcEntryInfoTree_GetNodePathRelative(mr_Model.GetCurrentNode(),mr_Model.GetRootNode(),strDest);

	std::list<CString> fileList;
	if(nID==ID_MENUITEM_ADD_FILE){		//ファイル追加
		//「全てのファイル」のフィルタ文字を作る
		CString strAnyFile(MAKEINTRESOURCE(IDS_FILTER_ANYFILE));
		std::vector<TCHAR> filter(strAnyFile.GetLength()+1);
		UtilMakeFilterString(strAnyFile,&filter[0],filter.size());

		CMultiFileDialog dlg(NULL, NULL, OFN_NOCHANGEDIR|OFN_DONTADDTORECENT|OFN_PATHMUSTEXIST|OFN_HIDEREADONLY|OFN_ALLOWMULTISELECT,&filter[0]);
		if(IDOK==dlg.DoModal()){
			//ファイル名取り出し
			CString tmp;
			if(dlg.GetFirstPathName(tmp)){
				do{
					fileList.push_back(tmp);
				}while(dlg.GetNextPathName(tmp));
			}
		}
	}else{		//フォルダ追加
		CLFFolderDialog dlg(m_hFrameWnd,NULL,BIF_RETURNONLYFSDIRS|BIF_NEWDIALOGSTYLE);
		if(IDOK==dlg.DoModal()){
			fileList.push_back(dlg.GetFolderPath());
		}
	}

	if(!fileList.empty()){
		//追加開始
		AddItems(fileList,strDest);
	}
}


void CFileListView::EnableDropTarget(bool bEnable)
{
	if(bEnable){
		//ドロップ受け入れ設定
		::RegisterDragDrop(m_hWnd,&m_DropTarget);
	}else{
		//ドロップを受け入れない
		::RevokeDragDrop(m_hWnd);
	}
}


//---------------------------------------------------------
//    IDropCommunicatorの実装:ドラッグ&ドロップによる圧縮
//---------------------------------------------------------
HRESULT CFileListView::DragEnter(IDataObject *lpDataObject,POINTL &pt,DWORD &dwEffect)
{
	return DragOver(lpDataObject,pt,dwEffect);
}

HRESULT CFileListView::DragLeave()
{
	//全てのハイライトを無効に
	SetItemState( -1, ~LVIS_DROPHILITED, LVIS_DROPHILITED);
	m_nDropHilight=-1;
	return S_OK;
}

HRESULT CFileListView::DragOver(IDataObject *,POINTL &pt,DWORD &dwEffect)
{
	//フォーマットに対応した処理をする
	if(!m_DropTarget.QueryFormat(CF_HDROP) || !mr_Model.IsAddItemsSupported()){	//ファイル専用
		//ファイルではないので拒否
		dwEffect = DROPEFFECT_NONE;
	}else{
		//---ドロップ先アイテムを取得
		CPoint ptTemp(pt.x,pt.y);
		ScreenToClient(&ptTemp);
		int nIndex=HitTest(ptTemp,NULL);

		//---ちらつきを押さえるため、前と同じアイテムがハイライトされるならハイライトをクリアしない
		if(nIndex!=m_nDropHilight){
			//全てのハイライトを無効に
			SetItemState( -1, ~LVIS_DROPHILITED, LVIS_DROPHILITED);
			m_nDropHilight=-1;
			ARCHIVE_ENTRY_INFO_TREE* lpNode=mr_Model.GetFileListItemByIndex(nIndex);
			if(lpNode){		//アイテム上にDnD
				//アイテムがフォルダだったらハイライト
				if(lpNode->bDir){
					SetItemState( nIndex, LVIS_DROPHILITED, LVIS_DROPHILITED);
					m_nDropHilight=nIndex;
				}
			}
		}
		dwEffect = DROPEFFECT_COPY;
	}
	return S_OK;
}

//ファイルのドロップ
HRESULT CFileListView::Drop(IDataObject *lpDataObject,POINTL &pt,DWORD &dwEffect)
{
	//全てのハイライトを無効に
	SetItemState( -1, ~LVIS_DROPHILITED, LVIS_DROPHILITED);
	m_nDropHilight=-1;

	//ファイル取得
	std::list<CString> fileList;
	if(S_OK==m_DropTarget.GetDroppedFiles(lpDataObject,fileList)){
		dwEffect = DROPEFFECT_COPY;

		//---ドロップ先を特定
		CPoint ptTemp(pt.x,pt.y);
		ScreenToClient(&ptTemp);
		int nIndex=HitTest(ptTemp,NULL);

		CString strDest;	//放り込む先
		ARCHIVE_ENTRY_INFO_TREE* lpNode=mr_Model.GetFileListItemByIndex(nIndex);
		if(lpNode){		//アイテム上にDnD
			//アイテムがフォルダだったらそのフォルダに追加
			if(lpNode->bDir){
				ArcEntryInfoTree_GetNodePathRelative(lpNode,mr_Model.GetRootNode(),strDest);
			}else{
				//カレントフォルダに追加
				ArcEntryInfoTree_GetNodePathRelative(mr_Model.GetCurrentNode(),mr_Model.GetRootNode(),strDest);
			}
		}else{	//アイテム外にDnD->カレントフォルダに追加
			ArcEntryInfoTree_GetNodePathRelative(mr_Model.GetCurrentNode(),mr_Model.GetRootNode(),strDest);
		}
		TRACE(_T("Target:%s\n"),(LPCTSTR)strDest);

		//追加開始
		return AddItems(fileList,strDest);
	}else{
		//受け入れできない形式
		dwEffect = DROPEFFECT_NONE;
		return S_FALSE;	//S_OK
	}
}

//-----------------------------
// ドラッグ&ドロップによる解凍
//-----------------------------

LRESULT CFileListView::OnBeginDrag(LPNMHDR pnmh)
{
	if(!mr_Model.IsExtractEachSupported()){
		//選択ファイルの解凍はサポートされていない
		//ErrorMessage(CString(MAKEINTRESOURCE(IDS_ERROR_FILELIST_EXTRACT_SELECTED_NOT_SUPPORTED)));
		MessageBeep(MB_ICONASTERISK);
		return 0;
	}

	if(!mr_Model.CheckArchiveExists()){	//存在しないならエラー
		return 0;
	}
	//選択されたアイテムを列挙
	std::list<ARCHIVE_ENTRY_INFO_TREE*> items;
	GetSelectedItems(items);
	if(items.empty()){	//本来あり得ない
		ASSERT(!"This code cannot be run");
		return 0;
	}

	if(!m_TempDirMgr.ClearSubDir()){
		//テンポラリディレクトリを空に出来ない
		ErrorMessage(CString(MAKEINTRESOURCE(IDS_ERROR_CANT_CLEAR_TEMPDIR)));
		return 0;
	}else{
		::EnableWindow(m_hFrameWnd,FALSE);

		//ドラッグ&ドロップで解凍
		CString strLog;
		HRESULT hr=m_DnDSource.DragDrop(mr_Model,items,mr_Model.GetCurrentNode(),m_TempDirMgr.GetDirPath(),strLog);
		if(FAILED(hr)){
			if(hr==E_ABORT){
				CLogDialog LogDialog;
				LogDialog.SetData(strLog);
				LogDialog.DoModal();
			}else{
				ErrorMessage(strLog);
			}
		}

		::EnableWindow(m_hFrameWnd,TRUE);
		::SetForegroundWindow(m_hFrameWnd);
	}
	return 0;
}

void CFileListView::OnSelectAll(UINT,int,HWND)
{
	SetItemState(-1, LVIS_SELECTED, LVIS_SELECTED);
	SetFocus();
}


void CFileListView::OnDelete(UINT uNotifyCode,int nID,HWND hWndCtrl)
{
	if(!mr_Model.IsDeleteItemsSupported()){
		//選択ファイルの削除はサポートされていない
		if(1==uNotifyCode){	//アクセラレータから操作
			MessageBeep(MB_OK);
		}else{
			ErrorMessage(CString(MAKEINTRESOURCE(IDS_ERROR_FILELIST_DELETE_SELECTED_NOT_SUPPORTED)));
		}
		return;// false;
	}

	//選択されたファイルを列挙
	std::list<ARCHIVE_ENTRY_INFO_TREE*> items;
	GetSelectedItems(items);

	//ファイルが選択されていなければエラー
	ASSERT(!items.empty());
	if(items.empty()){
		return;// false;
	}

	//消去確認
	if(IDYES!=MessageBox(CString(MAKEINTRESOURCE(IDS_ASK_FILELIST_DELETE_SELECTED)),UtilGetMessageCaption(),MB_YESNO|MB_DEFBUTTON2|MB_ICONEXCLAMATION)){
		return;
	}

	//----------------
	// ファイルを処理
	//----------------
	//ウィンドウを使用不可に
	::EnableWindow(m_hFrameWnd,FALSE);
	CString strLog;
	bool bRet=mr_Model.DeleteItems(items,strLog);
	//ウィンドウを使用可能に
	::EnableWindow(m_hFrameWnd,TRUE);
	SetForegroundWindow(m_hFrameWnd);
	if(!bRet){
		CLogDialog LogDlg;
		LogDlg.SetData(strLog);
		LogDlg.DoModal(m_hFrameWnd);
	}

	//アーカイブ内容更新
	::PostMessage(m_hFrameWnd,WM_FILELIST_REFRESH,NULL,NULL);
}

//コンテキストメニューを開く
void CFileListView::OnContextMenu(HWND hWndCtrl,CPoint &Point)
{
	//何も選択していなければ表示しない
	if(GetSelectedCount()==0)return;

	if(-1==Point.x&&-1==Point.y){
		//キーボードからの入力である場合
		//リストビューの左上に表示する
		Point.x=Point.y=0;
		ClientToScreen(&Point);
	}else{
		//マウス右ボタンがファイル一覧ウィンドウの*カラム*で押されたのであれば返る
		HDHITTESTINFO hdHitTestInfo={0};
		hdHitTestInfo.pt=Point;
		GetHeader().ScreenToClient(&hdHitTestInfo.pt);
		GetHeader().HitTest(&hdHitTestInfo);
		if(hdHitTestInfo.flags&HHT_ONHEADER){
			return;
		}
	}

	//部分解凍が使用できないなら、メニューを表示する意味がない
	//TODO:削除メニューはどうする?部分解凍できずに削除可能はほぼあり得ない
	if(!mr_Model.IsExtractEachSupported()){
		MessageBeep(MB_ICONASTERISK);
		return;
	}

	//---右クリックメニュー表示
	CMenu cMenu;
	cMenu.LoadMenu(IDR_FILELIST_POPUP);
	CMenuHandle cSubMenu(cMenu.GetSubMenu(0));

	//コマンドを追加するためのサブメニューを探す
	int MenuCount=cSubMenu.GetMenuItemCount();
	int iIndex=-1;
	for(int i=0;i<=MenuCount;i++){
		if(-1==cSubMenu.GetMenuItemID(i)){	//ポップアップの親
			iIndex=i;
			break;
		}
	}
	ASSERT(-1!=iIndex);
	if(-1!=iIndex){
		MenuCommand_MakeUserAppMenu(cSubMenu.GetSubMenu(iIndex));
		MenuCommand_MakeSendToMenu(cSubMenu.GetSubMenu(iIndex+1));
	}

	cSubMenu.TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,Point.x, Point.y, m_hWnd,NULL);
}

void CFileListView::OnOpenWithUserApp(UINT uNotifyCode,int nID,HWND hWndCtrl)
{
	if(nID<ID_MENUITEM_USERAPP_END){
		//LhaForge設定のコマンド
		OnUserApp(MenuCommand_GetCmdArray(),nID-ID_MENUITEM_USERAPP_BEGIN);
	}else{
		//SendToのコマンド
		OnSendToApp(nID-ID_MENUITEM_USERAPP_END);
	}
}


bool CFileListView::OnUserApp(const std::vector<CMenuCommandItem> &menuCommandArray,UINT nID)	//「プログラムで開く」のハンドラ
{
	ASSERT(!menuCommandArray.empty());
	ASSERT(nID<menuCommandArray.size());
	if(nID>=menuCommandArray.size())return false;

	if(!mr_Model.IsOK())return false;

	//選択されたアイテムを列挙
	std::list<ARCHIVE_ENTRY_INFO_TREE*> items;
	GetSelectedItems(items);

	std::list<CString> filesList;
	if(!items.empty()){
		CString strLog;
		if(!mr_Model.MakeSureItemsExtracted(NULL,mr_Model.GetRootNode(),items,filesList,false,strLog)){
			CLogDialog LogDialog;
			LogDialog.SetData(strLog);
			LogDialog.DoModal();
			return false;
		}
	}

	//---実行情報取得
	//パラメータ展開に必要な情報
	std::map<stdString,CString> envInfo;
	UtilMakeExpandInformation(envInfo);

	//コマンド・パラメータ展開
	CString strCmd,strParam,strDir;
	UtilExpandTemplateString(strCmd,  menuCommandArray[nID].Path, envInfo);	//コマンド
	UtilExpandTemplateString(strParam,menuCommandArray[nID].Param,envInfo);	//パラメータ
	UtilExpandTemplateString(strDir,  menuCommandArray[nID].Dir,  envInfo);	//ディレクトリ

	//引数置換
	if(-1!=strParam.Find(_T("%F"))){
		//ファイル一覧を連結して作成
		CString strFileList;
		for(std::list<CString>::iterator ite=filesList.begin();ite!=filesList.end();++ite){
			CPath path=*ite;
			path.QuoteSpaces();
			strFileList+=(LPCTSTR)path;
			strFileList+=_T(" ");
		}
		strParam.Replace(_T("%F"),strFileList);
		//---実行
		::ShellExecute(GetDesktopWindow(),NULL,strCmd,strParam,strDir,SW_SHOW);
	}else if(-1!=strParam.Find(_T("%S"))){
		for(std::list<CString>::iterator ite=filesList.begin();ite!=filesList.end();++ite){
			CPath path=*ite;
			path.QuoteSpaces();

			CString strParamTmp=strParam;
			strParamTmp.Replace(_T("%S"),(LPCTSTR)path);
			//---実行
			::ShellExecute(GetDesktopWindow(),NULL,strCmd,strParamTmp,strDir,SW_SHOW);
		}
	}else{
		::ShellExecute(GetDesktopWindow(),NULL,strCmd,strParam,strDir,SW_SHOW);
	}

	return true;
}

bool CFileListView::OnSendToApp(UINT nID)	//「プログラムで開く」のハンドラ
{
	ASSERT(MenuCommand_GetNumSendToCmd());
	ASSERT(nID<MenuCommand_GetNumSendToCmd());
	if(nID>=MenuCommand_GetNumSendToCmd())return false;

	if(!mr_Model.IsOK())return false;

	//---選択解凍開始
	//選択されたアイテムを列挙
	std::list<ARCHIVE_ENTRY_INFO_TREE*> items;
	GetSelectedItems(items);

	std::list<CString> filesList;
	if(!items.empty()){
		CString strLog;
		if(!mr_Model.MakeSureItemsExtracted(NULL,mr_Model.GetRootNode(),items,filesList,false,strLog)){
			CLogDialog LogDialog;
			LogDialog.SetData(strLog);
			LogDialog.DoModal();
			return false;
		}
	}

	//引数置換
	const std::vector<SHORTCUTINFO>& sendToCmd=MenuCommand_GetSendToCmdArray();
	if(PathIsDirectory(sendToCmd[nID].strCmd)){
		//対象はディレクトリなので、コピー
		CString strFiles;
		for(std::list<CString>::const_iterator ite=filesList.begin();ite!=filesList.end();++ite){
			CPath file=*ite;
			file.RemoveBackslash();
			strFiles+=file;
			strFiles+=_T('|');
		}
		strFiles+=_T('|');
		//TRACE(strFiles);
		std::vector<TCHAR> srcBuf(strFiles.GetLength()+1);
		UtilMakeFilterString(strFiles,&srcBuf[0],srcBuf.size());

		CPath destDir=sendToCmd[nID].strCmd;
		destDir.AddBackslash();
		//Windows標準のコピー動作
		SHFILEOPSTRUCT fileOp={0};
		fileOp.wFunc=FO_COPY;
		fileOp.fFlags=FOF_NOCONFIRMMKDIR|FOF_NOCOPYSECURITYATTRIBS|FOF_NO_CONNECTED_ELEMENTS;
		fileOp.pFrom=&srcBuf[0];
		fileOp.pTo=destDir;

		//コピー実行
		if(::SHFileOperation(&fileOp)){
			//エラー
			ErrorMessage(CString(MAKEINTRESOURCE(IDS_ERROR_FILE_COPY)));
			return false;
		}else if(fileOp.fAnyOperationsAborted){
			//キャンセル
			ErrorMessage(CString(MAKEINTRESOURCE(IDS_ERROR_USERCANCEL)));
			return false;
		}
		return true;
	}else{
		//送り先はプログラム
		//ファイル一覧を連結して作成
		CString strFileList;
		for(std::list<CString>::iterator ite=filesList.begin();ite!=filesList.end();++ite){
			CPath path=*ite;
			path.QuoteSpaces();
			strFileList+=(LPCTSTR)path;
			strFileList+=_T(" ");
		}
		CString strParam=sendToCmd[nID].strParam+_T(" ")+strFileList;
		//---実行
		CPath cmd=sendToCmd[nID].strCmd;
		cmd.QuoteSpaces();
		CPath workDir=sendToCmd[nID].strWorkingDir;
		workDir.QuoteSpaces();
		::ShellExecute(GetDesktopWindow(),NULL,cmd,strParam,workDir,SW_SHOW);
	}

	return true;
}

void CFileListView::OnExtractItem(UINT,int nID,HWND)
{
	//アーカイブと同じフォルダに解凍する場合はtrue
	const bool bSameDir=(ID_MENUITEM_EXTRACT_SELECTED_SAMEDIR==nID);

	if(!mr_Model.IsOK()){
		return;// false;
	}
	if(!mr_Model.IsExtractEachSupported()){
		//選択ファイルの解凍はサポートされていない
		ErrorMessage(CString(MAKEINTRESOURCE(IDS_ERROR_FILELIST_EXTRACT_SELECTED_NOT_SUPPORTED)));
		return;// false;
	}

	//選択されたアイテムを列挙
	std::list<ARCHIVE_ENTRY_INFO_TREE*> items;
	GetSelectedItems(items);
	if(items.empty()){
		//選択されたファイルがない
		ErrorMessage(CString(MAKEINTRESOURCE(IDS_ERROR_FILELIST_NOT_SELECTED)));
		return;// false;
	}

	//解凍
	CString strLog;
	HRESULT hr=mr_Model.ExtractItems(m_hFrameWnd,bSameDir,items,mr_Model.GetCurrentNode(),strLog);

	SetForegroundWindow(m_hFrameWnd);

	if(FAILED(hr)){
		CLogDialog LogDialog;
		LogDialog.SetData(strLog);
		LogDialog.DoModal();
	}
}

void CFileListView::OnFindItem(UINT uNotifyCode,int nID,HWND hWndCtrl)
{
	if(ID_MENUITEM_FINDITEM_END==nID){
		mr_Model.EndFindItem();
	}else{
		CString strSpec;
		if(UtilInputText(CString(MAKEINTRESOURCE(IDS_INPUT_FIND_PARAM)),strSpec)){
			mr_Model.EndFindItem();
			ARCHIVE_ENTRY_INFO_TREE* lpFound=mr_Model.FindItem(strSpec,mr_Model.GetCurrentNode());
			mr_Model.SetCurrentNode(lpFound);
		}
	}
}

void CFileListView::OnShowCustomizeColumn(UINT,int,HWND)
{
	//カラムヘッダ編集メニューを表示するため、カラムヘッダの右クリックをエミュレート
	BOOL bTemp;
	NMHDR nmhdr;
	nmhdr.hwndFrom=GetHeader();
	OnColumnRClick(0, &nmhdr, bTemp);
}
