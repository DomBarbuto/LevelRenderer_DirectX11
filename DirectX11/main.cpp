
#include "renderer.h" // example rendering code (not Gateware code!)


// lets pop a window and use D3D11 to clear to a green screen
int main()
{
	// Initialize the clock
	Clock timer;

	GWindow win;
	GEventResponder msgs;
	GDirectX11Surface d3d11;
	if (+win.Create(0, 0, m_windowWidth, m_windowHeight, GWindowStyle::WINDOWEDBORDERED))
	{
		win.SetWindowName("Dominic Barbuto - Programming Assignment 2");
		float clr[] = { 33/255.0f, 43/255.0f, 78/255.0f, 1 }; // start with a neon green
		msgs.Create([&](const GW::GEvent& e) {
			GW::SYSTEM::GWindow::Events q;
			if (+e.Read(q) && q == GWindow::Events::RESIZE)
				clr[2] += 0.01f; // move towards a cyan as they resize
		});
		win.Register(msgs);
		if (+d3d11.Create(win, GW::GRAPHICS::DEPTH_BUFFER_SUPPORT))
		{
			Renderer renderer(win, d3d11);
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
					con->ClearRenderTargetView(view, clr);
					con->ClearDepthStencilView(depth, D3D11_CLEAR_DEPTH, 1, 0);
					renderer.UpdateCamera(timer);
					if (timer.duration.count() > m_frameTime)
					{
						//std::cout << "time since last draw = " << timer.duration.count() /1000.0f << std::endl;

						// Start the clock
						timer.start = timer.now = timer.clock.now();
						renderer.Render();
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