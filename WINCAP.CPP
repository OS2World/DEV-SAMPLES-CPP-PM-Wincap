#define INCL_WININPUT
#define INCL_WINWINDOWMGR
#include <os2.h>
#include <iapp.hpp>
#include <ithread.hpp>
#include <iframe.hpp>
#include <icmdhdr.hpp>
#include <imoushdr.hpp>
#include <imenuhdr.hpp>
#include <imenubar.hpp>
#include <isubmenu.hpp>
#include <ititle.hpp>
#include <igbitmap.hpp>
#include <igrafctx.hpp>
#include <igpyline.hpp>
#include <igbundle.hpp>
#include <ivport.hpp>
#include <ibmpctl.hpp>
#include <iinfoa.hpp>
#include <ifiledlg.hpp>
#include <imsgbox.hpp>
#include <iexcbase.hpp>

#define IDM_FILE          300
#define IDM_FILE_SELWIN   301
#define IDM_FILE_SELAREA  302
#define IDM_FILE_SAVE     303
#define IDM_FILE_CLEAR    304
#define IDM_FILE_EXIT     310
#define IDM_OPTION        400
#define IDM_OPT_HIDE      401
#define IDM_OPT_ACTWIN    402
#define IDM_OPT_FIT       403
#define IDM_HELP          500
#define IDM_HELP_ABOUT    501

class MainWin : public IFrameWindow,
                public ICommandHandler,
                public IMouseHandler,
                public IMenuHandler
{
   enum  SelectMode { normal, selectWin, selectArea };

   public:
      MainWin();
      ~MainWin();

   protected:
      Boolean command (ICommandEvent& evt);
      Boolean mouseClicked (IMouseClickEvent& evt);
      Boolean mouseMoved (IMouseEvent& evt);
      Boolean menuShowing (IMenuEvent& menuEvent, ISubmenu& submenuAboutToShow);

   private:
      void drawRect(const IRectangle& rect, Boolean bDragFence = false);
      IWindowHandle windowUnderPoint (const IPoint& hitPoint);
      static IRectangle windowRect(const IWindowHandle& hWnd);

      IViewPort*        pViewPort;
      IBitmapControl*   pBmpCtl;
      IGBitmap*         pBitmap;
      IMenuBar*         pMenuBar;
      IGPolyline*       pGPolyline;
      SelectMode        selectMode;
      IPoint            anchorPt;
      IPoint            movingPt;
      IWindowHandle     hWndTarget;
};

MainWin :: MainWin ()
         : pBmpCtl (NULL),
           pBitmap (NULL),
           hWndTarget (NULLHANDLE),
           selectMode (normal),
           anchorPt (-1, -1),
           movingPt (-1, -1)
{
   pMenuBar = new IMenuBar (this);
   pMenuBar->addText (IDM_FILE, "~File");
   pMenuBar->addSubmenu (IDM_FILE);
   pMenuBar->addText (IDM_FILE_SELWIN, "Select ~Window", IDM_FILE);
   pMenuBar->addText (IDM_FILE_SELAREA, "Select ~Area", IDM_FILE);
   pMenuBar->addText (IDM_FILE_SAVE, "~Save Bitmap...", IDM_FILE);
   pMenuBar->addText (IDM_FILE_CLEAR, "~Clear", IDM_FILE);
   pMenuBar->addText (IDM_FILE_EXIT, "~Exit", IDM_FILE);
   pMenuBar->addText (IDM_OPTION, "~Options");
   pMenuBar->addSubmenu (IDM_OPTION);
   pMenuBar->addText (IDM_OPT_HIDE, "~Hide Before Select", IDM_OPTION);
   pMenuBar->addText (IDM_OPT_ACTWIN, "~Activate Selected Win", IDM_OPTION);
   pMenuBar->addText (IDM_OPT_FIT, "~Stretch To Fit", IDM_OPTION);
   pMenuBar->addText (IDM_HELP, "~Help");
   pMenuBar->addSubmenu (IDM_HELP);
   pMenuBar->addText (IDM_HELP_ABOUT, "~About", IDM_HELP);
   pViewPort = new IViewPort (101, this, this);
   setClient (pViewPort);
   setIcon (ISystemPointerHandle(ISystemPointerHandle::question));
   ITitle title (this, "Screen Capture Utility");

   pGPolyline = new IGPolyline (IPointArray(0, NULL));

   ICommandHandler::handleEventsFor (this);
   IMouseHandler::handleEventsFor (this);
   IMenuHandler::handleEventsFor (this);
}

MainWin :: ~MainWin()
{
   delete pMenuBar;
   delete pViewPort;
   delete pBitmap;
   delete pBmpCtl;
   delete pGPolyline;
}

Boolean MainWin :: command(ICommandEvent& evt)
{
   switch(evt.commandId())
   {
      case IDM_FILE_SELWIN :
      case IDM_FILE_SELAREA :
      {
         capturePointer();
         if (evt.commandId() == IDM_FILE_SELWIN)
         {
            ISystemPointerHandle pointer (ISystemPointerHandle::question);
            setMousePointer (ISystemPointerHandle(pointer));
            hWndTarget = NULLHANDLE;
            selectMode = selectWin;
         }
         else
         {
            ISystemPointerHandle pointer (ISystemPointerHandle::move);
            setMousePointer (pointer);
            anchorPt = movingPt = IPoint (-1, -1);
            selectMode = selectArea;
         }
         if (pMenuBar->isItemChecked(IDM_OPT_HIDE))
            hide();
      }

      case IDM_FILE_CLEAR :    //case above falls through in this.
      {
         delete pBitmap;
         delete pBmpCtl;
         pBitmap = NULL;
         pBmpCtl = NULL;
         pViewPort->setLayoutDistorted (IWindow::layoutChanged |
                                        IWindow::immediateUpdate, 0);
         break;
      }

      case IDM_OPT_HIDE :
      case IDM_OPT_ACTWIN :
      case IDM_OPT_FIT :
      {
         if (pMenuBar->isItemChecked(evt.commandId()))
            pMenuBar->uncheckItem (evt.commandId());
         else
            pMenuBar->checkItem (evt.commandId());
         if (evt.commandId() == IDM_OPT_FIT && pBmpCtl && pBitmap)
         {
            if (pMenuBar->isItemChecked(IDM_OPT_FIT))
               pBmpCtl->sizeTo (pViewPort->size());
            else
               pBmpCtl->sizeTo (pBitmap->size());
            pViewPort->setLayoutDistorted (IWindow::layoutChanged |
                                           IWindow::immediateUpdate, 0);
         }
         break;
      }

      case IDM_FILE_SAVE :
      {
         IFileDialog::Settings  settings;
         settings.setFileName ("*.bmp");
         settings.setSaveAsDialog ();
         IFileDialog *fileDlg = new IFileDialog (NULL, this,
            IFileDialog::defaultStyle(), settings);
         IString fileName = fileDlg->fileName ();
         Boolean bOKPressed = fileDlg->pressedOK ();
         delete fileDlg;
         if (bOKPressed)
            pBitmap->writeToFile (fileName, IGBitmap::Bitmap);
         break;
      }

      case IDM_HELP_ABOUT :
      {
         IMessageBox* pMsgBox = new IMessageBox (this);
         pMsgBox->show ("Copies window/screen contents to a bitmap.\n\n"
               "Use \"Select Window\" menu option and click on the area "
               "of the desired window.\n"
               "OR Use \"Select Area\" menu option and select a "
               "rectangular area on the desktop by dragging mouse button1.\n\n"
               "Use \"Save Bitmap\" menu option to write to a BMP file.",
               IMessageBox::informationIcon |
               IMessageBox::okButton |
               IMessageBox::moveable);
         delete pMsgBox;
         break;
      }

      case DID_CANCEL : //ESC key.
      {
         if (selectMode == selectWin || selectMode == selectArea)
         {
            show ();
            releasePointer();
            setMousePointer (NULLHANDLE);
            if (selectMode == selectWin && hWndTarget != NULLHANDLE)
               drawRect (windowRect(hWndTarget));                //Erase rect.
            if (selectMode == selectArea && movingPt != IPoint(-1, -1))
               drawRect (IRectangle(anchorPt, movingPt), true);  //Erase rect.
            selectMode = normal;
            hWndTarget = NULLHANDLE;
            anchorPt = movingPt = IPoint(-1, -1);
            break;
         }
      }

      case IDM_FILE_EXIT :
      {
         close();
         break;
      }

      default :
         return false;
   }

   return true;
}

Boolean MainWin :: mouseClicked(IMouseClickEvent& evt)
{
   IPoint mousePos = evt.mousePosition();

   if (selectMode == selectArea &&
       evt.mouseButton() == IMouseClickEvent::button1 &&
       evt.mouseAction() == IMouseClickEvent::down)
   {
      anchorPt = IWindow::mapPoint (mousePos, handle(), HWND_DESKTOP);
      movingPt = IPoint(-1, -1);
      if (pMenuBar->isItemChecked(IDM_OPT_ACTWIN))
      {
         IWindowHandle hWnd = windowUnderPoint (mousePos);
         if (hWnd != NULLHANDLE)
            WinSetActiveWindow (HWND_DESKTOP, hWnd);
      }
      return false;
   }

   if ((selectMode == selectWin || selectMode == selectArea) &&
        evt.mouseButton() == IMouseClickEvent::button1 &&
        evt.mouseAction() == IMouseClickEvent::up)
   {
      releasePointer();
      setMousePointer (NULLHANDLE);
      if ((selectMode == selectArea && movingPt != IPoint(-1, -1)) ||
          (selectMode == selectWin && hWndTarget != NULLHANDLE))
      {
         IRectangle bmpRect;
         if (selectMode == selectArea)
         {
            drawRect (IRectangle(anchorPt, movingPt), true);  //Erase rect.
            bmpRect = IRectangle (anchorPt, movingPt);
         }
         else
         {
            drawRect (windowRect(hWndTarget));                //Erase rect.
            bmpRect = windowRect (hWndTarget);
         }
         //Include boundaries.
         bmpRect.sizeTo (IPair(bmpRect.width()+1, bmpRect.height()+1));
         IPresSpaceHandle hScreenPS = WinGetScreenPS (HWND_DESKTOP);
         pBitmap = new IGBitmap (hScreenPS, bmpRect);
         pBmpCtl = new IBitmapControl (100, pViewPort, pViewPort,
                                       pBitmap->handle (),
                                       IRectangle (),
                                       IBitmapControl::defaultStyle());
         if (pMenuBar->isItemChecked(IDM_OPT_FIT))
            pBmpCtl->sizeTo (pViewPort->size());
         else
            pBmpCtl->sizeTo (pBitmap->size());
         pViewPort->setLayoutDistorted (IWindow::layoutChanged |
                                        IWindow::immediateUpdate, 0);
         WinReleasePS (hScreenPS);
      }
      selectMode = normal;
      hWndTarget = NULLHANDLE;
      anchorPt = movingPt = IPoint(-1, -1);
      show ();
   }

   return false;
}

Boolean MainWin :: mouseMoved (IMouseEvent& evt)
{
   if (selectMode == selectWin)
   {
      IWindowHandle hWndNewTarget = windowUnderPoint (evt.mousePosition());
      //Ignore WM_MOUSEMOVE if a WM_PAINT is coming for hWndNewTarget because
      //WM_PAINT erases my rectangle I am going to draw around hWndNewTarget.
      QMSG qmsg;
      if (WinPeekMsg (IThread::current().anchorBlock(), &qmsg, hWndNewTarget,
                      WM_PAINT, WM_PAINT, PM_NOREMOVE))
         return false;
      if (hWndNewTarget != NULLHANDLE && hWndNewTarget != hWndTarget)
      {
         if (hWndTarget != NULLHANDLE)
            drawRect (windowRect(hWndTarget));  //Erase previous rect.
         hWndTarget = hWndNewTarget;
         if (pMenuBar->isItemChecked(IDM_OPT_ACTWIN))
            WinSetActiveWindow (HWND_DESKTOP, hWndTarget);
         drawRect (windowRect(hWndTarget));  //Draw new rect.
     }
   }

   if (selectMode == selectArea && anchorPt != IPoint(-1, -1))
   {
      if (movingPt != IPoint(-1, -1))
         drawRect (IRectangle(anchorPt, movingPt), true); //Erase previous rect.
      movingPt = IWindow::mapPoint(evt.mousePosition(), handle(), HWND_DESKTOP);
      drawRect (IRectangle(anchorPt, movingPt), true);    //Draw new rect.
   }

   return false;
}

void MainWin :: drawRect(const IRectangle& rect, Boolean bDragFence)
{
   IPresSpaceHandle screenPS = WinGetScreenPS(HWND_DESKTOP);
   IGraphicContext screenGC (screenPS);

   IGraphicBundle::MixMode oldMixMode = screenGC.mixMode ();
   IColor oldPenColor = screenGC.penColor ();
   unsigned long oldPenWidth = screenGC.penWidth ();
   IGraphicBundle::PenType oldPenType = screenGC.penType ();

   screenGC.setMixMode (IGraphicBundle::xor);
   screenGC.setPenColor (IColor(IColor::red));
   if (bDragFence)
      screenGC.setPenType (IGraphicBundle::dot);
   else
      screenGC.setPenWidth (2);

   IPoint points[5];
   points[0] = rect.bottomLeft();
   points[1] = rect.bottomRight();
   points[2] = rect.topRight();
   points[3] = rect.topLeft();
   points[4] = rect.bottomLeft();
   pGPolyline->setPoints (IPointArray(5, points));
   pGPolyline->drawOn (screenGC);

   screenGC.setMixMode (oldMixMode);
   screenGC.setPenColor (oldPenColor);
   screenGC.setPenWidth (oldPenWidth);
   screenGC.setPenType (oldPenType);
   WinReleasePS (screenPS);
}

IRectangle MainWin :: windowRect (const IWindowHandle& hWnd)
{
   SWP swp;
   WinQueryWindowPos (hWnd, &swp);
   IPoint origin = IWindow::mapPoint(IPoint(0, 0), hWnd, HWND_DESKTOP);
   return IRectangle (origin, ISize(swp.cx-1, swp.cy-1));
}

IWindowHandle MainWin :: windowUnderPoint (const IPoint& hitPoint)
{
   IPoint pt = IWindow::mapPoint (hitPoint, handle(), HWND_DESKTOP);
   POINTL ptl = pt.asPOINTL();
   return WinWindowFromPoint (HWND_DESKTOP, &ptl, true);
}

Boolean MainWin :: menuShowing (IMenuEvent& menuEvent, ISubmenu& subMenu)
{
   if (menuEvent.menuItemId() == IDM_FILE)
   {
      subMenu.enableItem (IDM_FILE_SAVE, pBitmap != NULL);
      subMenu.enableItem (IDM_FILE_CLEAR, pBitmap != NULL);
   }

   return false;
}

main()
{
   MainWin* pMainWin = NULL;

   try
   {
      pMainWin = new MainWin;
      pMainWin->sizeTo (ISize(300, 250));
      pMainWin->show();
      pMainWin->setFocus();
      IApplication::current().run();
      delete pMainWin;
   } catch (IException& exc)
   {
      WinAlarm (HWND_DESKTOP, WA_ERROR);
      IInfoArea* pInfoArea = new IInfoArea (pMainWin);
      pInfoArea->setInactiveText (exc.text());
      IApplication::current().run();
      delete pInfoArea;
      delete pMainWin;
   }
}

