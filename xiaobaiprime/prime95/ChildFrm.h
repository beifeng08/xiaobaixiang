// Copyright (c) 2026 beifeng08
// Licensed under the MIT License. See LICENSE in the project root.

#pragma once

class CChildFrame : public CMDIChildWnd
{
	DECLARE_DYNCREATE(CChildFrame)
public:
	CChildFrame();

public:

public:

	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdateFrameTitle(BOOL bAddToTitle);

public:
	virtual ~CChildFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	DECLARE_MESSAGE_MAP()
};
