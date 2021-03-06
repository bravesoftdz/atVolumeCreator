#ifndef atFetchImageThreadH
#define atFetchImageThreadH
#include "mtkThread.h"
#include "mtkStringList.h"
#include "atRenderClientUtils.h"
//---------------------------------------------------------------------------

using mtk::StringList;
using mtk::Thread;

typedef void __fastcall (__closure *RCCallBack)(void);
class RenderClient;
class FetchImageThread : public mtk::Thread
{
	public:
							                FetchImageThread(RenderClient& rc);
		void				                setup(const string& url, const string& cacheFolder);
		virtual void                        run();
        void				                assignUrl(const string& url);
		bool				                setCacheRoot(const string& cr);
		void				                worker();
        											//!The on Image callback is called when image data is retrieved from the server
        RCCallBack							onImage;

	private:
    	string								mImageURL;
        string								mCacheRootFolder;

        					                //A renderclient is host, manager for this thread. Give it memory retrieved
        RenderClient&		                mRenderClient;

};

#endif
