// Copyright (c) 2026 OmniSensor
// 本实现参考了公开文献与社区实践

#define MAX_VIEW_LINES	1000

class CSysCorePrimeView : public CScrollView
{
protected: 
	CSysCorePrimeView();
	DECLARE_DYNCREATE(CSysCorePrimeView)

public:
	CSysCorePrimeDoc* GetDocument();

public:

	public:
	virtual void OnDraw(CDC* pDC);  
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	virtual void OnUpdate(CView* pSender, LPARAM lHint, CObject* pHint);
	protected:
	
public:
	virtual ~CSysCorePrimeView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	char	BaseTitle[80];		
	char	Title[80];		
	char	LineData[MAX_VIEW_LINES][200]; 
	char	*Lines[MAX_VIEW_LINES];	
	int	NumLines;		
	int	MaxLineSize;		
	HICON	icon;			

public:
	void base_title (const char *);
	void title (const char *);
	void ChangeIcon (int);
	void getCharSize ();
	void position (int, int, BOOL);
	void LineFeed ();
	void RealOutputStr (const char *);

protected:
	
	afx_msg void OnScroll();
	afx_msg void OnTitle();
	afx_msg void OnIcon();
	afx_msg void OnEditCopy();
	
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnDestroy();
};

#ifndef _DEBUG  
inline CSysCorePrimeDoc* CSysCorePrimeView::GetDocument()
   { return (CSysCorePrimeDoc*)m_pDocument; }
#endif

void PositionViews (int forceTile);
void SaveViews (void);

