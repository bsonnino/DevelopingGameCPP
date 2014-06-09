// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) Microsoft Corporation. All rights reserved

#include "pch.h"
#include "Game.h"

#include "..\Common\DirectXHelper.h"
#include "HitTestingHelpers.h"

using namespace StarterKit;

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Platform;
using namespace Windows::Foundation;
using namespace VSD3DStarter;

// Loads vertex and pixel shaders from files and instantiates the cube geometry.
Game::Game(const std::shared_ptr<DX::DeviceResources>& deviceResources) :
m_loadingComplete(false),
m_deviceResources(deviceResources)
{
	CreateDeviceDependentResources();
	CreateWindowSizeDependentResources();
}

// Loads meshes and other resources that depend on the device, but not on window size.
void Game::CreateDeviceDependentResources()
{
	m_graphics.Initialize(m_deviceResources->GetD3DDevice(), m_deviceResources->GetD3DDeviceContext(), m_deviceResources->GetDeviceFeatureLevel());

	// Set DirectX to not cull any triangles so the entire mesh will always be shown.
	CD3D11_RASTERIZER_DESC d3dRas(D3D11_DEFAULT);
	d3dRas.CullMode = D3D11_CULL_NONE;
	d3dRas.MultisampleEnable = true;
	d3dRas.AntialiasedLineEnable = true;

	ComPtr<ID3D11RasterizerState> p3d3RasState;
	m_deviceResources->GetD3DDevice()->CreateRasterizerState(&d3dRas, &p3d3RasState);
	m_deviceResources->GetD3DDeviceContext()->RSSetState(p3d3RasState.Get());

	// Load the scene objects.
	auto loadMeshTask = Mesh::LoadFromFileAsync(
		m_graphics,
		L"gamelevel.cmo",
		L"",
		L"",
		m_meshModels)
		.then([this]()
	{
		return Mesh::LoadFromFileAsync(
			m_graphics,
			L"field.cmo",
			L"",
			L"",
			m_meshModels,
			false  // Do not clear the vector of meshes
			);
	}).then([this]()
	{
		return Mesh::LoadFromFileAsync(
			m_graphics,
			L"soccer_goal.cmo",
			L"",
			L"",
			m_meshModels,
			false  // Do not clear the vector of meshes
			);
	}).then([this]()
	{
		return Mesh::LoadFromFileAsync(
			m_graphics,
			L"goalkeeper.cmo",
			L"",
			L"",
			m_meshModels,
			false  // Do not clear the vector of meshes
			);
	});

	(loadMeshTask).then([this]()
	{
		// Scene is ready to be rendered.
		m_loadingComplete = true;
	});
}

// Initializes view parameters when the window size changes.
void Game::CreateWindowSizeDependentResources()
{
	m_miscConstants.ViewportHeight = m_deviceResources->GetScreenViewport().Height;
	m_miscConstants.ViewportWidth = m_deviceResources->GetScreenViewport().Width;
	m_graphics.UpdateMiscConstants(m_miscConstants);

	m_graphics.GetCamera().SetOrientationMatrix(m_deviceResources->GetOrientationTransform3D());

	Size outputSize = m_deviceResources->GetOutputSize();

	// Setup the camera parameters for our scene.
	m_graphics.GetCamera().SetViewport((UINT) outputSize.Width, (UINT) outputSize.Height);
	m_graphics.GetCamera().SetPosition(XMFLOAT3(25.0f, 10.0f, 0.0f));
	m_graphics.GetCamera().SetLookAt(XMFLOAT3(100.0f, 0.0f, 0.0f));
	float aspectRatio = outputSize.Width / outputSize.Height;
	float fovAngleY = 30.0f * XM_PI / 180.0f;

	if (aspectRatio < 1.0f)
	{
		// Portrait or snap view
		m_graphics.GetCamera().SetUpVector(XMFLOAT3(1.0f, 0.0f, 0.0f));
		fovAngleY = 120.0f * XM_PI / 180.0f;
	}
	else
	{
		// Landscape view.
		m_graphics.GetCamera().SetUpVector(XMFLOAT3(0.0f, 1.0f, 0.0f));
	}

	m_graphics.GetCamera().SetProjection(fovAngleY, aspectRatio, 1.0f, 100.0f);

	// Setup lighting for our scene.
	static const XMVECTORF32 s_vPos = { 5.0f, 5.0f, -2.5f, 0.f };

	XMFLOAT4 dir;
	DirectX::XMStoreFloat4(&dir, XMVector3Normalize(s_vPos));

	m_lightConstants.ActiveLights = 1;
	m_lightConstants.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_lightConstants.IsPointLight[0] = false;
	m_lightConstants.LightColor[0] = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	m_lightConstants.LightDirection[0] = dir;
	m_lightConstants.LightSpecularIntensity[0].x = 2;

	m_graphics.UpdateLightConstants(m_lightConstants);
}

// Release resources that depend on the device. Called when the device is released.
void Game::ReleaseDeviceDependentResources()
{
	for (Mesh* m : m_meshModels)
	{
		delete m;
	}
	m_meshModels.clear();

	m_loadingComplete = false;
}

// Called once per frame, updates the scene state.
void Game::Update(DX::StepTimer const& timer)
{
	// Rotate scene.
	m_rotation = static_cast<float>(timer.GetTotalSeconds()) * 0.5f;
	auto totalTime = (float) fmod(timer.GetTotalSeconds(), 2.3f);
	m_translationX = 63.0f + 11.5f * totalTime;
	m_translationY = 11.5f * totalTime - 5 * totalTime*totalTime;
	m_translationZ = 3 * totalTime;
}

// Renders one frame using the Starter Kit helpers.
void Game::Render()
{
	// Loading is asynchronous. Only draw geometry after it's loaded.
	if (!m_loadingComplete)
	{
		return;
	}

	auto context = m_deviceResources->GetD3DDeviceContext();

	// Set render targets to the screen.
	auto rtv = m_deviceResources->GetBackBufferRenderTargetView();
	auto dsv = m_deviceResources->GetDepthStencilView();
	ID3D11RenderTargetView *const targets[1] = { rtv };
	context->OMSetRenderTargets(1, targets, dsv);

	// Draw our scene models.
	XMMATRIX rotation = XMMatrixRotationY(m_rotation);
	rotation *= XMMatrixTranslation(m_translationX, m_translationY, m_translationZ);
	auto goalTransform = XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixRotationY(-XM_PIDIV2)* XMMatrixTranslation(85.5f, -0.5, 0);
	auto goalkeeperTransform = XMMatrixTranslation(85.65f, 1.4f, m_goalkeeperPosition);

	for (UINT i = 0; i < m_meshModels.size(); i++)
	{
		XMMATRIX modelTransform = rotation;

		String^ meshName = ref new String(m_meshModels[i]->Name());

		m_graphics.UpdateMiscConstants(m_miscConstants);

		if (String::CompareOrdinal(meshName, L"Sphere_Node") == 0)
			m_meshModels[i]->Render(m_graphics, modelTransform);
		else if (String::CompareOrdinal(meshName, L"Plane_Node") == 0)
			m_meshModels[i]->Render(m_graphics, XMMatrixIdentity());
		else if (String::CompareOrdinal(meshName, L"Cube_Node") == 0)
			m_meshModels[i]->Render(m_graphics, goalkeeperTransform);
		else
			m_meshModels[i]->Render(m_graphics, goalTransform);
	}

}

// Starts the glow animation for an object.
void Game::ToggleHitEffect(String^ object)
{
}

// Changes an object's diffuse material color.
void Game::ChangeMaterialColor(String^ object, float r, float g, float b)
{
	for (Mesh* m : m_meshModels)
	{
		String^ meshName = ref new String(m->Name());
		if (String::CompareOrdinal(object, meshName) == 0)
		{
			m->Materials()[0].Diffuse[0] = r;
			m->Materials()[0].Diffuse[1] = g;
			m->Materials()[0].Diffuse[2] = b;
			break;
		}
	}
}

// Hit tests the scene on the point (x,y), looking for objects. 
// If there's any valid hit, returns the name of the hit object.
String^ Game::OnHitObject(int x, int y)
{
	String^ result = nullptr;

	XMFLOAT3 point;
	XMFLOAT3 dir;
	m_graphics.GetCamera().GetWorldLine(x, y, &point, &dir);

	XMFLOAT4X4 world;
	XMMATRIX worldMat = XMMatrixRotationY(m_rotation);
	XMStoreFloat4x4(&world, worldMat);

	float closestT = FLT_MAX;
	for (Mesh* m : m_meshModels)
	{
		XMFLOAT4X4 meshTransform = world;

		auto name = ref new String(m->Name());

		float t = 0;
		bool hit = HitTestingHelpers::LineHitTest(*m, &point, &dir, &meshTransform, &t);
		if (hit && t < closestT)
		{
			result = name;
		}
	}

	return result;
}

void Game::OnKeyDown(Windows::System::VirtualKey key)
{
	const float MaxGoalkeeperPosition = 6.0;
	const float MinGoalkeeperPosition = -6.0;
	if (key == Windows::System::VirtualKey::Right)
		m_goalkeeperPosition = m_goalkeeperPosition >= MaxGoalkeeperPosition ?
	m_goalkeeperPosition : m_goalkeeperPosition + 0.1f;
	else if (key == Windows::System::VirtualKey::Left)
		m_goalkeeperPosition = m_goalkeeperPosition <= MinGoalkeeperPosition ?
	m_goalkeeperPosition : m_goalkeeperPosition - 0.1f;
}

