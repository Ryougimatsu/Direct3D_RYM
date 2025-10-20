#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;
#include "direct3d.h"
#include "debug_ostream.h"
#include <fstream>
#include "shader_field.h"

struct VS_CONSTANT_BUFFER_AMBIENT
{
	XMFLOAT4 ambient_color;
};

struct VS_CONSTANT_BUFFER_DIRECTIONAL
{
	XMFLOAT4 directional_vector;
	XMFLOAT4 directional_color;
};

namespace
{
	ID3D11VertexShader* g_pVertexShader = nullptr;
	ID3D11InputLayout* g_pInputLayout = nullptr;
	ID3D11Buffer* g_pVSConstantBuffer0 = nullptr;
	ID3D11Buffer* g_pVSConstantBuffer1 = nullptr;
	ID3D11Buffer* g_pVSConstantBuffer2 = nullptr;
	ID3D11Buffer* g_pVSConstantBuffer3 = nullptr;
	ID3D11Buffer* g_pVSConstantBuffer4 = nullptr;
	ID3D11PixelShader* g_pPixelShader = nullptr;
	ID3D11SamplerState* g_pSamplerState = nullptr;

	// 注意！初期化で外部から設定されるもの。Release不要。
	ID3D11Device* g_pDevice = nullptr;
	ID3D11DeviceContext* g_pContext = nullptr;
}

bool Shader_field_Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	HRESULT hr; // HRESULTはDirectXの関数の戻り値で、成功か失敗かを示す	

	// デバイスとデバイスコンテキストのチェック
	if (!pDevice || !pContext) {
		hal::dout << "Shader_field_Initialize() : 渡されたデバイスやコンテキストが不正です" << std::endl;
		return false;
	}

	// デバイスとデバイスコンテキストの保存
	g_pDevice = pDevice;
	g_pContext = pContext;


	// 事前コンパイル済み頂点シェーダーの読み込み
	std::ifstream ifs_vs("resource/shader/shader_vertex_field.cso", std::ios::binary);

	if (!ifs_vs) {
		MessageBox(nullptr, "shader_vertex_field.csoが見つかりません", "error", MB_OK);
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
	hr = g_pDevice->CreateVertexShader(vsbinary_pointer, filesize, nullptr, &g_pVertexShader);

	if (FAILED(hr)) {
		hal::dout << "Shader_field_Initialize() : 頂点シェーダーの作成に失敗しました" << std::endl;
		delete[] vsbinary_pointer; // メモリを解放
		return false;
	}


	//頂点レイアウトの定義
	D3D11_INPUT_ELEMENT_DESC layout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "NORMAL",   0, DXGI_FORMAT_R32G32B32_FLOAT,    0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,       0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};

	UINT num_elements = ARRAYSIZE(layout); // 配列の要素数を取得

	//頂点レイアウトの作成
	hr = g_pDevice->CreateInputLayout(layout, num_elements, vsbinary_pointer, filesize, &g_pInputLayout);

	delete[] vsbinary_pointer; // バイナリデータのバッファを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_field_Initialize() : 頂点レイアウトの作成に失敗しました" << std::endl;
		return false;
	}


	// 定数バッファの作成
	D3D11_BUFFER_DESC buffer_desc{};
	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4); // バッファのサイズは行列一つ分
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER; // 定数バッファとして使用
	buffer_desc.Usage = D3D11_USAGE_DEFAULT; // 書き換えはほとんどしない

	buffer_desc.ByteWidth = sizeof(XMFLOAT4X4);
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer0); // World
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer1); // View
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer2); // Projection

	// b3 (Ambient)
	buffer_desc.ByteWidth = sizeof(VS_CONSTANT_BUFFER_AMBIENT); // 16 bytes
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer3);

	// b4 (Directional)
	buffer_desc.ByteWidth = sizeof(VS_CONSTANT_BUFFER_DIRECTIONAL); // 32 bytes
	g_pDevice->CreateBuffer(&buffer_desc, nullptr, &g_pVSConstantBuffer4);

	// ピクセルシェーダーの読み込み
	std::ifstream ifs_ps("resource/shader/shader_pixel_field.cso", std::ios::binary);
	if (!ifs_ps) {
		MessageBox(nullptr, "shader_pixel_field.csoが見つかりません", "error", MB_OK);
		return false;
	}

	ifs_ps.seekg(0, std::ios::end);
	filesize = ifs_ps.tellg();
	ifs_ps.seekg(0, std::ios::beg);

	unsigned char* psbinary_pointer = new unsigned char[filesize];
	ifs_ps.read((char*)psbinary_pointer, filesize);
	ifs_ps.close();

	// ピクセルシェーダーの作成
	hr = g_pDevice->CreatePixelShader(psbinary_pointer, filesize, nullptr, &g_pPixelShader);

	delete[] psbinary_pointer; // メモリを解放

	if (FAILED(hr)) {
		hal::dout << "Shader_field_Initialize() : ピクセルシェーダーの作成に失敗しました" << std::endl;
		return false;
	}

	D3D11_SAMPLER_DESC sampDesc = {};
	sampDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	sampDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP; // 設為 WRAP (平鋪)
	sampDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP; // 設為 WRAP (平鋪)
	sampDesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sampDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	sampDesc.MinLOD = 0;
	sampDesc.MaxLOD = D3D11_FLOAT32_MAX;
	hr = g_pDevice->CreateSamplerState(&sampDesc, &g_pSamplerState);
	if (FAILED(hr)) {
		hal::dout << "Shader_field_Initialize() : 採樣器狀態的創建に失敗しました" << std::endl;
		return false;
	}

	return true;
}



void Shader_field_Finalize()
{
	SAFE_RELEASE(g_pSamplerState);
	SAFE_RELEASE(g_pPixelShader);
	SAFE_RELEASE(g_pVSConstantBuffer0);
	SAFE_RELEASE(g_pVSConstantBuffer1);
	SAFE_RELEASE(g_pVSConstantBuffer2);
	SAFE_RELEASE(g_pVSConstantBuffer3);
	SAFE_RELEASE(g_pVSConstantBuffer4);
	SAFE_RELEASE(g_pInputLayout);
	SAFE_RELEASE(g_pVertexShader);
}

void Shader_field_SetMatrix(const DirectX::XMMATRIX& matrix)
{
	// シェーダーに渡すために行列を転置する
	XMFLOAT4X4 transpose;

	// XMMATRIXからXMFLOAT4X4へ格納する際に行列を転置
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// 定数バッファを更新
	g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);
}

void Shader_field_SetWorldMatrix(const DirectX::XMMATRIX& matrix)
{
	// シェーダーに渡すために行列を転置する
	XMFLOAT4X4 transpose;

	// XMMATRIXからXMFLOAT4X4へ格納する際に行列を転置
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// ワールド行列用の定数バッファを更新
	g_pContext->UpdateSubresource(g_pVSConstantBuffer0, 0, nullptr, &transpose, 0, 0);

}

void Shader_field_SetViewMatrix(const DirectX::XMMATRIX& matrix)
{
	// シェーダーに渡すために行列を転置する
	XMFLOAT4X4 transpose;

	// XMMATRIXからXMFLOAT4X4へ格納する際に行列を転置
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// ビュー行列用の定数バッファを更新
	g_pContext->UpdateSubresource(g_pVSConstantBuffer1, 0, nullptr, &transpose, 0, 0);

}

void Shader_field_SetProjectMatrix(const DirectX::XMMATRIX& matrix)
{
	// シェーダーに渡すために行列を転置する
	XMFLOAT4X4 transpose;

	// XMMATRIXからXMFLOAT4X4へ格納する際に行列を転置
	XMStoreFloat4x4(&transpose, XMMatrixTranspose(matrix));

	// プロジェクション行列用の定数バッファを更新
	g_pContext->UpdateSubresource(g_pVSConstantBuffer2, 0, nullptr, &transpose, 0, 0);

}

void Shader_field_SetAmbientColor(const XMFLOAT4& color)
{
	VS_CONSTANT_BUFFER_AMBIENT data;
	data.ambient_color = color;
	g_pContext->UpdateSubresource(g_pVSConstantBuffer3, 0, nullptr, &data, 0, 0);
}

void Shader_field_SetDirectionalLight(const XMFLOAT4& vector, const XMFLOAT4& color)
{
	VS_CONSTANT_BUFFER_DIRECTIONAL data;
	data.directional_vector = vector;
	data.directional_color = color;
	g_pContext->UpdateSubresource(g_pVSConstantBuffer4, 0, nullptr, &data, 0, 0);
}

void Shader_field_Begin()
{
	//頂点シェーダーとピクセルシェーダーを描画パイプラインに設定
	g_pContext->VSSetShader(g_pVertexShader, nullptr, 0);
	g_pContext->PSSetShader(g_pPixelShader, nullptr, 0);

	//頂点レイアウトを描画パイプラインに設定
	g_pContext->IASetInputLayout(g_pInputLayout);

	//定数バッファを描画パイプラインに設定
	g_pContext->VSSetConstantBuffers(0, 1, &g_pVSConstantBuffer0);
	g_pContext->VSSetConstantBuffers(1, 1, &g_pVSConstantBuffer1);
	g_pContext->VSSetConstantBuffers(2, 1, &g_pVSConstantBuffer2);
	g_pContext->VSSetConstantBuffers(3, 1, &g_pVSConstantBuffer3);
	g_pContext->VSSetConstantBuffers(4, 1, &g_pVSConstantBuffer4);

	//サンプラーステートを描画パイプラインに設定
	g_pContext->PSSetSamplers(0, 1, &g_pSamplerState);
}
