
#include "renderer.h" // example rendering code (not Gateware code!)

// lets pop a window and use D3D11 to clear to a green screen
int main()
{
	GWindow win;
	GEventResponder msgs;
	GDirectX11Surface d3d11;
	std::string assignmentString = "Dominic Barbuto | Level Renderer | ";
	std::string fpsString = "FPS: 0";

	// Initialize the clock
	Clock gameTimer;
	Clock fpsTimer;
	Clock levelSwitchTimer;

	if (+win.Create(0, 0, m_windowWidth, m_windowHeight, GWindowStyle::WINDOWEDBORDERED))
	{
		// Set Program name
		win.SetWindowName((assignmentString + fpsString).c_str());

		// Set back buffer color
		float clr[] = { 13/255.0f, 18/255.0f, 43/255.0f, 1 }; // Dark, night sky color

		// Create event responder
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
			{
				clr[2] += 0.01f; // move towards a cyan as they resize
			}
		});
		win.Register(msgs);

		if (+d3d11.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		{
			Renderer renderer(win, d3d11);
			gameTimer.Start();
			fpsTimer.Start();
			levelSwitchTimer.Start();
			while (+win.ProcessWindowEvents())
			{
				IDXGISwapChain* swap = nullptr;
				ID3D11DeviceContext* con = nullptr;
				ID3D11RenderTargetView * view = nullptr;
				ID3D11DepthStencilView* depth = nullptr;
				if (+d3d11.GetImmediateContext((void**)&con) &&
					+d3d11.GetRenderTargetView((void**)&view) &&
					+d3d11.GetDepthStencilView((void**)&depth) &&
					+d3d11.GetSwapchain((void**)&swap))
				{
					// Calculate time since last frame, and reset gameFPS timer
					double dt = gameTimer.GetMSElapsed();
					gameTimer.Restart();

					// Check for F1 key press, open file dialog to select new level
					if (GetAsyncKeyState(VK_F1))
					{
						GameManager* gm = renderer.GetGameManager();
						gm->SwitchLevel();
						renderer.ReInitializeBuffers();
						renderer.BeginMusic();
					}

					con->ClearRenderTargetView(view, clr);
					con->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH, 1, 0);
					renderer.UpdateCamera(dt);
					renderer.Render();
					
					// Update the FPS count on window menu bar every 1 second.
					static int fpsCount = 0;
					fpsCount++;
					if (fpsTimer.GetMSElapsed() > 1000) // 1000ms == 1 sec
					{
						fpsString = "FPS: " + std::to_string(fpsCount);
						win.SetWindowName((assignmentString + fpsString).c_str());
						fpsCount = 0;
						fpsTimer.Restart();
					}
					
					swap->Present(1, 0);
					// release incremented COM reference counts
					swap->Release();
					view->Release();
					depth->Release();
					con->Release();
				}
			}
		}
	}
	return 0; 
}

//void CheckForLevelSwitch(Renderer& renderer, GW::SYSTEM::GWindow& win)
//{
//	// Check for F1 Key for selecting another level
//	if (GetAsyncKeyState(VK_F1))
//	{
//		// Set up OPENFILENAME structure which will hold all of the 
//		// info about our selected file
//		
//		GameManager* gm = renderer.GetGameManager();
//		OPENFILENAME ofn;
//		// Interchanging between LPWSTR and char[]
//		char text[200] = "";
//		wchar_t wtext[200];
//		mbstowcs(wtext, text, strlen(text) + 1);//Plus null
//		LPWSTR fileName = wtext;
//		ZeroMemory(&ofn, sizeof(ofn));
//
//		ofn.lStructSize = sizeof(OPENFILENAME);
//		UNIVERSAL_WINDOW_HANDLE handle;
//		win.GetWindowHandle(handle);
//		ofn.hwndOwner = (HWND)handle.window; // try null for hwndOwner
//		ofn.lpstrFilter = L"Text files only(*.txt)\0*.txt;\0";
//		ofn.lpstrFile = fileName;
//		ofn.nMaxFile = MAX_PATH;
//		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
//		
//		// If the user selected a file
//		if (GetOpenFileName(&ofn))
//		{
//			char fileNameString[200];
//			wcstombs(fileNameString, ofn.lpstrFile, 200);
//
//			// Now must parse out the unwanted characters
//			std::string result(fileNameString);
//			size_t length = std::strlen(result.c_str());
//
//			size_t lastIndexOf = result.rfind("\\");
//			std::string subString = result.substr(lastIndexOf + 1, length - lastIndexOf);
//			std::string finalResult = "../Levels/" + subString;
//
//			gm->gameLevelPath = finalResult;
//			gm->SwitchLevel();
//			renderer.ReInitializeBuffers();
//			renderer.BeginMusic(gm->gameLevelPath);
//
//		}
//	}
//}

