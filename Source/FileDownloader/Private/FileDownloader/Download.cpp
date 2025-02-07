// Copyright 2023 LandscapeCombinator. All Rights Reserved.

#include "FileDownloader/Download.h"
#include "FileDownloader/LogFileDownloader.h"
#include "FileDownloader/FileDownloaderStyle.h"

#include "Async/Async.h"
#include "Http.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformFileManager.h" 
#include "Widgets/Notifications/SProgressBar.h"
#include "Misc/ScopedSlowTask.h"


#define LOCTEXT_NAMESPACE "FLandscapeCombinatorModule"

float SleepSeconds = 0.05;
float TimeoutSeconds = 10;
TMap<FString, int> ExpectedSizeCache; 

FString Shorten(FString Input)
{
	if (Input.Len() > 83)
	{
		return Input.Left(40) + "..." + Input.Right(40);
	}
	else
	{
		return Input;
	}
}

// FIXME: use TAtomic<bool> for bTriggered, but with TAtomic<bool> bTriggered { false}, everything blocks
// We use bTriggered to avoid OnProcessRequestComplete being invoked multiple times

bool Download::SynchronousFromURL(FString URL, FString File)
{
	UE_LOG(LogFileDownloader, Log, TEXT("Downloading '%s' to '%s'"), *URL, *File);

	if (ExpectedSizeCache.Contains(URL))
	{
		UE_LOG(LogFileDownloader, Log, TEXT("Cache says expected size for '%s' is '%d'"), *URL, ExpectedSizeCache[URL]);
		return SynchronousFromURLExpecting(URL, File, ExpectedSizeCache[URL]);
	}
	else
	{
		TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(URL);
		Request->SetVerb("HEAD");
		Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");

		bool bIsComplete = false;
		int32 ExpectedSize = 0;
		
		bool *bTriggered = new bool(false);
		Request->OnProcessRequestComplete().BindLambda([URL, File, &ExpectedSize, &bIsComplete, bTriggered](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
			if (*bTriggered) return;
			*bTriggered = true;
			if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
			{
				ExpectedSize = FCString::Atoi(*Response->GetHeader("Content-Length"));
			}
			bIsComplete = true;
		});
		Request->ProcessRequest();

		float StartTime = FPlatformTime::Seconds();
		UE_LOG(LogFileDownloader, Log, TEXT("Actively waiting for Content-Length request to complete"));
		while (!bIsComplete && FPlatformTime::Seconds() - StartTime <= TimeoutSeconds)
		{
			FPlatformProcess::Sleep(SleepSeconds);
		}
		UE_LOG(LogFileDownloader, Log, TEXT("Finished waiting for Content-Length request to complete"));

		Request->CancelRequest();

		return SynchronousFromURLExpecting(URL, File, ExpectedSize);
	}
}

bool Download::SynchronousFromURLExpecting(FString URL, FString File, int32 ExpectedSize)
{
	IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.FileExists(*File) && ExpectedSize != 0 && PlatformFile.FileSize(*File) == ExpectedSize)
	{
		UE_LOG(LogFileDownloader, Log, TEXT("File already exists with the correct size, skipping download of '%s' to '%s' "), *URL, *File);
		return true;
	}

	bool bDownloadResult = false;
	bool bIsComplete = false;

	TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
	Request->SetURL(URL);
	Request->SetVerb("GET");
	Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
	bool *bTriggered = new bool(false);
	Request->OnProcessRequestComplete().BindLambda([URL, File, &bDownloadResult, &bIsComplete, bTriggered](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
	{
		if (*bTriggered) return;
		*bTriggered = true;
		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
		bool DownloadSuccess = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
		bool SavedFile = false;
		if (DownloadSuccess)
		{
			SavedFile = FFileHelper::SaveArrayToFile(Response->GetContent(), *File);
			if (SavedFile)
			{
				AddExpectedSize(URL, PlatformFile.FileSize(*File));
				bDownloadResult = true;
				bIsComplete = true;

				UE_LOG(LogFileDownloader, Log, TEXT("Finished downloading '%s' to '%s'"), *URL, *File);
			}
			else
			{
				bDownloadResult = false;
				bIsComplete = true;
				UE_LOG(LogFileDownloader, Error, TEXT("Error while saving '%s' to '%s'"), *URL, *File);
			}
		}
		else
		{
			UE_LOG(LogFileDownloader, Error, TEXT("Error while downloading '%s' to '%s'"), *URL, *File);
			UE_LOG(LogFileDownloader, Error, TEXT("Request was not successful. Error %d."), Response->GetResponseCode());
			bDownloadResult = false;
			bIsComplete = true;
		}
	});
	Request->ProcessRequest();
		

	float StartTime = FPlatformTime::Seconds();
	UE_LOG(LogFileDownloader, Log, TEXT("Actively waiting for download to complete"));
	while (!bIsComplete && FPlatformTime::Seconds() - StartTime <= TimeoutSeconds)
	{
		FPlatformProcess::Sleep(SleepSeconds);
	}
	UE_LOG(LogFileDownloader, Log, TEXT("Finished waiting for download to complete"));
		
	Request->CancelRequest();

	return bDownloadResult;
}

void Download::FromURL(FString URL, FString File, TFunction<void(bool)> OnComplete)
{
	UE_LOG(LogFileDownloader, Log, TEXT("Downloading from URL '%s' to '%s'"), *URL, *File);

	if (ExpectedSizeCache.Contains(URL))
	{
		UE_LOG(LogFileDownloader, Log, TEXT("Cache says expected size for '%s' is '%d'"), *URL, ExpectedSizeCache[URL]);
		FromURLExpecting(URL, File, ExpectedSizeCache[URL], OnComplete);
	}
	else
	{
		UE_LOG(LogFileDownloader, Log, TEXT("No cache for '%s'"), *URL);
		TSharedRef<IHttpRequest> Request = FHttpModule::Get().CreateRequest();
		Request->SetURL(URL);
		Request->SetVerb("HEAD");
		Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
		bool *bTriggered = new bool(false);

		FScopedSlowTask *Task = new FScopedSlowTask(0, LOCTEXT("Download::FromURL", "Fetching Content Length from URL"));
		Task->MakeDialog();

		Request->OnProcessRequestComplete().BindLambda([Task, URL, File, OnComplete, bTriggered](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful) {
			if (*bTriggered) return;
			*bTriggered = true;

			Task->Destroy();

			if (bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode()))
			{
				int64 ExpectedSize = FCString::Atoi64(*Response->GetHeader("Content-Length"));
				FromURLExpecting(URL, File, ExpectedSize, OnComplete);
			}
			else
			{
				FromURLExpecting(URL, File, 0, OnComplete);
			}
		});
		Request->ProcessRequest();
	}

}

void Download::FromURLExpecting(FString URL, FString File, int64 ExpectedSize, TFunction<void(bool)> OnComplete)
{
	// make sure we are in game thread to spawn download progress windows
	AsyncTask(ENamedThreads::GameThread, [=]()
	{
		double *Downloaded = new double();

		TSharedPtr<SWindow> Window = SNew(SWindow)
			.SizingRule(ESizingRule::Autosized)
			.AutoCenter(EAutoCenter::PrimaryWorkArea)
			.Title(LOCTEXT("DownloadProgress", "Download Progress"));

		IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

		if (ExpectedSize != 0 && PlatformFile.FileExists(*File))
		{
			int64 FileSize = PlatformFile.FileSize(*File);
			if (FileSize == ExpectedSize)
			{
				UE_LOG(LogFileDownloader, Log, TEXT("File already exists with the correct size, skipping download of '%s' to '%s' "), *URL, *File);
				if (OnComplete) OnComplete(true);
				return;
			}
			else
			{
				UE_LOG(LogFileDownloader, Log, TEXT("File already exists with size %ld but the expected size is %ld. Redownloading"), FileSize, ExpectedSize);
			}
		}

		TSharedPtr<IHttpRequest> Request = FHttpModule::Get().CreateRequest().ToSharedPtr();
		Request->SetURL(URL);
		Request->SetVerb("GET");
		Request->SetHeader("User-Agent", "X-UnrealEngine-Agent");
		Request->OnRequestProgress().BindLambda([Downloaded](FHttpRequestPtr Request, int32 Sent, int32 Received) {
			*Downloaded = Received;
		});
		bool *bTriggered = new bool(false);
		Request->OnProcessRequestComplete().BindLambda([URL, File, OnComplete, Window, bTriggered](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
		{
			if (*bTriggered) return;
			*bTriggered = true;

			IPlatformFile &PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
			bool DownloadSuccess = bWasSuccessful && Response.IsValid() && EHttpResponseCodes::IsOk(Response->GetResponseCode());
			bool SavedFile = false;
			if (DownloadSuccess)
			{
				SavedFile = FFileHelper::SaveArrayToFile(Response->GetContent(), *File);
				if (SavedFile)
				{
					AddExpectedSize(URL, PlatformFile.FileSize(*File));
					UE_LOG(LogFileDownloader, Log, TEXT("Finished downloading '%s' to '%s'"), *URL, *File);
				}
				else
				{
					UE_LOG(LogFileDownloader, Error, TEXT("Error while saving '%s' to '%s'"), *URL, *File);
				}
			}
			else
			{
				UE_LOG(LogFileDownloader, Error, TEXT("Error while downloading '%s' to '%s'"), *URL, *File);
				if (Response.IsValid())
				{
					UE_LOG(LogFileDownloader, Error, TEXT("Request was not successful. Error %d."), Response->GetResponseCode());
				}
			}
			if (OnComplete) OnComplete(DownloadSuccess && SavedFile);
			Window->RequestDestroyWindow();
		});


		Request->ProcessRequest();

		Window->SetContent(
			SNew(SBox).Padding(FMargin(30, 30, 30, 30))
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot().AutoHeight()
				[
					SNew(STextBlock).Text(
						FText::Format(
							LOCTEXT("DowloadingURL", "Dowloading {0} to {1}."),
							FText::FromString(Shorten(URL)),
							FText::FromString(Shorten(File))
						)
					).Font(FFileDownloaderStyle::RegularFont())
				]
				+SVerticalBox::Slot().AutoHeight().Padding(FMargin(0, 0, 0, 20))
				[
					SNew(SProgressBar)
					.Percent_Lambda([Downloaded, ExpectedSize, Request]() {
						if (ExpectedSize) return *Downloaded / ExpectedSize;
						return *Downloaded / MAX_int32;
					})
					.RefreshRate(0.1)
				]
				+SVerticalBox::Slot().AutoHeight().HAlign(EHorizontalAlignment::HAlign_Center)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot().AutoWidth().HAlign(EHorizontalAlignment::HAlign_Center)
					[
						SNew(SButton)
						.OnClicked_Lambda([Window, Request]()->FReply {
							Request->CancelRequest();
							Window->RequestDestroyWindow();
							return FReply::Handled();
						})
						[
							SNew(STextBlock).Font(FFileDownloaderStyle::RegularFont()).Text(FText::FromString(" Cancel "))
						]
					]
				]
			]
		);
		Window->SetOnWindowClosed(FOnWindowClosed::CreateLambda([Request](const TSharedRef<SWindow>& Window) {
			Request->CancelRequest();
		}));
		FSlateApplication::Get().AddWindow(Window.ToSharedRef());
		if (Request->GetStatus() != EHttpRequestStatus::Processing)
		{
			Window->RequestDestroyWindow();
		}
	});
}

void Download::AddExpectedSize(FString URL, int32 ExpectedSize)
{
	if (!ExpectedSizeCache.Contains(URL))
	{
		ExpectedSizeCache.Add(URL, ExpectedSize);
		SaveExpectedSizeCache();
	}
}

FString Download::ExpectedSizeCacheFile()
{
	IPlatformFile::GetPlatformPhysical().CreateDirectory(*FPaths::ProjectSavedDir());
	return FPaths::Combine(FPaths::ProjectSavedDir(), "ExpectedSizeCache"); 
}

void Download::LoadExpectedSizeCache()
{
	FString CacheFile = ExpectedSizeCacheFile();
	//UE_LOG(LogFileDownloader, Log, TEXT("Loading expected size cache from '%s'"), *CacheFile);
		
	FArchive* FileReader = IFileManager::Get().CreateFileReader(*CacheFile);
	if (FileReader)
	{
		*FileReader << ExpectedSizeCache;
		FileReader->Close();
	}
}

void Download::SaveExpectedSizeCache()
{
	FString CacheFile = ExpectedSizeCacheFile();
	//UE_LOG(LogFileDownloader, Log, TEXT("Saving expected size cache to '%s'"), *CacheFile);
	FArchive* FileWriter = IFileManager::Get().CreateFileWriter(*CacheFile);
	if (FileWriter)
	{
		*FileWriter << ExpectedSizeCache;
		if (FileWriter->Close()) return;
	}

	UE_LOG(LogFileDownloader, Error, TEXT("Failed to save expected size cache to '%s'"), *CacheFile);
}

#undef LOCTEXT_NAMESPACE
