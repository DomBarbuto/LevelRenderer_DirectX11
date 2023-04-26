
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
					double dt = gameTimer.GetMSElapsed();
					gameTimer.Restart();

					con->ClearRenderTargetView(view, clr);
					con->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH, 1, 0);
					renderer.UpdateCamera(dt);
					renderer.Render();

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
	return 0; // that's all folks
}