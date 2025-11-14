#include "shader_billboard.h"
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "Sampler.h"


namespace
{
	ID3D11VertexShader* g_pVertexShader = nullptr;
	ID3D11InputLayout* g_pInputLayout = nullptr;
	ID3D11Buffer* g_pVSConstantBuffer0 = nullptr;
	ID3D11Buffer* g_pVSConstantBuffer1 = nullptr;
	ID3D11Buffer* g_pVSConstantBuffer2 = nullptr;
	ID3D11Buffer* g_pPSConstantBuffer0 = nullptr;
	ID3D11PixelShader* g_pPixelShader = nullptr;
}


bool Shader_Billboard_Initialize()
{
	HRESULT hr; // HRESULTはDirectXの関数の戻り値で、成功か失敗かを示す	

	// 事前コンパイル済み頂点シェーダーの読み込み
	std::ifstream ifs_vs("resource/shader/shader_vertex_billboard.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, "shader_vertex_billboard.csoが見つかりません", "error", MB_OK);
		return false;
	}

	// ファイルサイズを取得
	ifs_vs.seekg(0, std::ios::end); // ファイルポインタを終端に移動
	std::streamsize filesize = ifs_vs.tellg(); // ファイルポインタの位置を取得（つまりファイルサイズ）
	ifs_vs.seekg(0, std::ios::beg); // ファイルポインタを先頭に戻す

	// バイナリデータを格納するためのバッファを確保
	unsigned char* vsbinary_pointer = new unsigned char[filesize];

	ifs_vs.read((char*)vsbinary_pointer, filesize); // バイナリデータを読み込む
	ifs_vs.close(); // ファイルを閉じる

	// 頂点シェーダーの作成
	hr = Direct3D_GetDevice()->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "Shader_Billboard_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer; // メモリを解放
		return false;
	}


	//頂点レイアウトの定義
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

	//頂点レイアウトの作成
	hr = Direct3D_GetDevice()->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

	delete[] vsbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_Billboard_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}


	// 定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズは行列一つ分
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // 定数バッファとして使用

	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0); // World
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1); // View
	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2); // Projection


	// ピクセルシェーダーの読み込み
	std::ifstream ifs_ps("resource/shader/shader_pixel_billboard.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "shader_pixel_billboard.csoが見つかりません", "error", MB_OK);
		return false;
	}

	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	// ピクセルシェーダーの作成
	hr = Direct3D_GetDevice()->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);

	delete[] psbinary_pointer; // メモリを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_3D_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
		return false;
	}


	buffer_desc.ByteWidth = sizeof(XMFLOAT4); // バッファのサイズは行列一つ分

	Direct3D_GetDevice()->CreateBuffer(&buffer_desc, nullptr, &g_pPSConstantBuffer0); // World

	return true;
}

void Shader_Billboard_Finalize()
{
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer1);
	SAFE_RELEASE(g_pVSConstantBuffer2);
	SAFE_RELEASE(g_pPSConstantBuffer0);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
}

void Shader_Billboard_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// シェーダーに渡すために行列を転置する
	XMFLOAT4X4 transpose;

	// XMMATRIXからXMFLOAT4X4へ格納する際に行列を転置
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファを更新
	Direct3D_GetDeviceContext()->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void Shader_Billboard_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
	// シェーダーに渡すために行列を転置する
	XMFLOAT4X4 transpose;

	// XMMATRIXからXMFLOAT4X4へ格納する際に行列を転置
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// ワールド行列用の定数バッファを更新
	Direct3D_GetDeviceContext()->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &transpose, 0, 0);
}

void Shader_Billboard_SetProjectMatrix(const DirectX::XMMATRIX& matrix)
{
	// シェーダーに渡すために行列を転置する
	XMFLOAT4X4 transpose;

	// XMMATRIXからXMFLOAT4X4へ格納する際に行列を転置
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// ビュー行列用の定数バッファを更新
	Direct3D_GetDeviceContext()->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &transpose, 0, 0);
}

void Shader_Billboard_SetColor(const DirectX::XMFLOAT4& color)
{
	Direct3D_GetDeviceContext()->UpdateSubresource(g_pPSConstantBuffer0, 0, nullptr, &color, 0, 0);
}

void Shader_Billboard_Begin()
{
	//頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	Direct3D_GetDeviceContext()->VSSetShader(g_pVertexShader, nullptr, 0);
	Direct3D_GetDeviceContext()->PSSetShader(g_pPixelShader, nullptr, 0);

	//頂点レイアウトを描画パイプラインに設定
	Direct3D_GetDeviceContext()->IASetInputLayout(g_pInputLayout);

	//定数バッファを描画パイプラインに設定
	Direct3D_GetDeviceContext()->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
	Direct3D_GetDeviceContext()->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);
	Direct3D_GetDeviceContext()->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2);
	Direct3D_GetDeviceContext()->PSSetConstantBuffers(0, 1, &g_pPSConstantBuffer0);
}
