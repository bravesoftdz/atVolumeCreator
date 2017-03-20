#pragma hdrstop
#include "atFetchImagesThread.h"
#include "mtkLogger.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include "Poco/File.h"
#include "mtkFileUtils.h"
//---------------------------------------------------------------------------
using namespace mtk;

string getImageCacheFileNameAndPathFromURL(const string& url, const string& cacheRootFolder)
{
    vector<string> cachePaths = splitStringAtWord(url, "/owner/");
    if(cachePaths.size() == 2)
    {
		string fldr = substitute(cachePaths[1],"/","\\\\");
		return joinPath(cacheRootFolder, fldr, "image.tiff");
    }

   	return "";
}

int getImageZFromURL(const string& url)
{
    vector<string> cachePaths = splitStringAtWord(url, "/z/");
    if(cachePaths.size() == 2)
    {
    	//Extract
		cachePaths = splitString(cachePaths[1], '/');
        if(cachePaths.size() > 2)
        {
        	return mtk::toInt(cachePaths[1]);
        }
    }
    return -1;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}


static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)userp;

  mem->memory = (char*) realloc(mem->memory, mem->size + realsize + 1);

  if(mem->memory == NULL)
  {
    /* out of memory! */
    Log(lError) << "Not enough memory (realloc returned NULL)\n";
    return 0;
  }

  memcpy(&(mem->memory[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

FetchImagesThread::FetchImagesThread(const StringList& urls, const string& cacheRoot)
:
mImageURLs(urls),
mCacheRootFolder(cacheRoot)
{}

bool FetchImagesThread::setCacheRoot(const string& cr)
{
	//Check if path exists, if not try to create it
	mCacheRootFolder = cr;
    if(folderExists(mCacheRootFolder))
    {
    	return true;
    }
    else
    {
    	return createFolder(mCacheRootFolder);
    }
}

void FetchImagesThread::assignUrls(const StringList& urls)
{
	mImageURLs = urls;
}

void FetchImagesThread::run()
{
    worker();
}

void FetchImagesThread::worker()
{
	mIsRunning = true;
    while(!mIsTimeToDie)
    {
		Log(lInfo) << "Started Image fetching thread..";

        curl_global_init(CURL_GLOBAL_ALL);

	    for(int i = 0; i < mImageURLs.count(); i++)
	    {
	    	string url = mImageURLs[i];

            //Check cache first. if already in cache, don't fetch
            string outFilePathANDFileName = getImageCacheFileNameAndPathFromURL(url, mCacheRootFolder);
           	Poco::File f(outFilePathANDFileName);
            if(fileExists(outFilePathANDFileName) && f.getSize() > 200)
            {
            	Log(lInfo) << "File is in cache";
            }
            else
			{
                Log(lInfo) << "Thread is fetching: "<<getImageZFromURL(url);

                CURL *curl_handle;
                CURLcode res;

                struct MemoryStruct chunk;

                chunk.memory = (char*) malloc(1);  /* will be grown as needed by the realloc above */
                chunk.size = 0;    /* no data at this point */

                curl_global_init(CURL_GLOBAL_ALL);

                /* init the curl session */
                curl_handle = curl_easy_init();

                /* specify URL to get */
                curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());

                /* send all data to this function  */
                curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);

                /* we pass our 'chunk' struct to the callback function */
                curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);

                /* some servers don't like requests that are made without a user-agent
                   field, so we provide one */
                curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");

                /* get it! */
                res = curl_easy_perform(curl_handle);

                /* check for errors */
                if(res != CURLE_OK)
                {
                    Log(lError) << "curl_easy_perform() failed: " <<  curl_easy_strerror(res);
                }
                else
                {
                  /*
                   * chunk.memory points to a memory block that is chunk.size
                   * bytes big and contains the remote file.
                   */
                    string out = getImageCacheFileNameAndPathFromURL(url, mCacheRootFolder);
                    //Make sure path exists, if not create it
                    if(createFolder(getFilePath(out)))
                    {
                        ofstream of( out.c_str(), std::ofstream::binary);
                        of.write(&chunk.memory[0], chunk.size);

                        Log(lInfo) <<  (long)chunk.size << " bytes retrieved\n";
                        of.close();
                    }
                    else
                    {
                        Log(lError) << "Failed to write filePath: "<<out;
                    }
                }

                /* cleanup curl stuff */
                curl_easy_cleanup(curl_handle);
                free(chunk.memory);

                /* we're done with libcurl, so clean it up */
                curl_global_cleanup();
            }

        }

        mIsTimeToDie = true;


//        //First check if we already is having this data
//        //This will fetch from DB, or, if present, from the cache
//        TMemoryStream* imageMem = rs.getImage(z);
//
//        if(imageMem)
//        {
//            Image1->Picture->Graphic->LoadFromStream(imageMem);
//
//            //Save to local box folder
//            Image1->Invalidate();
//			outName.str("");
//            outName << mVolumesFolder->getValue() <<"\\" << rs.getProjectName() <<"\\"<<mScaleE->getValue()<<"\\"<<createZeroPaddedString(z, 4)<<".tif";
//            string in = rs.getImageLocalPathAndFileName();
//            //Make sure path exists, if not create it
//            if(createFolder(getFilePath(outName.str())))
//            {
//                if(convertTiff(in, outName.str()))
//                {
//                    Log(lInfo) << "Converted file: "<<in<<" to "<<outName;
//                    addTiffToStack(stackFName, outName.str());
//                }
//            }
//        }
//	    rs.clearImageMemory();
//        Application->ProcessMessages();
//    }


	}
  	Log(lInfo) << "Finished Image fetching thread..";
    mIsRunning = false;
    mIsFinished = true;

}

void FetchImagesThread::setup(const StringList& urls, const string& cacheFolder)
{
	mImageURLs = urls;
    mCacheRootFolder = cacheFolder;
}