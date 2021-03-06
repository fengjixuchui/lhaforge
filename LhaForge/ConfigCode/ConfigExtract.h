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

#pragma once

enum OUTPUT_TO;
enum CREATE_OUTPUT_DIR;
struct CConfigExtract:public IConfigConverter{
public:
	OUTPUT_TO OutputDirType;
	CString OutputDir;
	BOOL OpenDir;
	CREATE_OUTPUT_DIR CreateDir;
	BOOL ForceOverwrite;
	BOOL RemoveSymbolAndNumber;
	BOOL CreateNoFolderIfSingleFileOnly;
	BOOL LimitExtractFileCount;
	int MaxExtractFileCount;
	BOOL DeleteArchiveAfterExtract;	//正常に解凍できた圧縮ファイルを削除
	BOOL MoveToRecycleBin;			//解凍後ファイルをごみ箱に移動
	BOOL DeleteNoConfirm;			//確認せずに削除/ごみ箱に移動
	BOOL ForceDelete;				//解凍エラーを検知できない場合も削除
	BOOL DeleteMultiVolume;			//マルチボリュームもまとめて削除
	BOOL MinimumPasswordRequest;	//パスワード入力回数を最小にするならTRUE

	CString DenyExt;		//解凍対象から外す拡張子
protected:
	virtual void load(CONFIG_SECTION&);	//設定をCONFIG_SECTIONから読み込む
	virtual void store(CONFIG_SECTION&)const;	//設定をCONFIG_SECTIONに書き込む
public:
	virtual ~CConfigExtract(){}
	virtual void load(CConfigManager&);
	virtual void store(CConfigManager&)const;
};

