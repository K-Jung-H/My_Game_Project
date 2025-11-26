#pragma once



namespace Vector3
{
	inline XMFLOAT3 Lerp(const XMFLOAT3& v0, const XMFLOAT3& v1, float t)
	{
		XMFLOAT3 result;
		XMVECTOR xmV0 = XMLoadFloat3(&v0);
		XMVECTOR xmV1 = XMLoadFloat3(&v1);
		XMStoreFloat3(&result, XMVectorLerp(xmV0, xmV1, t));
		return result;
	}

	inline XMVECTOR GetRotationBetweenVectors(XMVECTOR start, XMVECTOR dest)
	{
		start = XMVector3Normalize(start);
		dest = XMVector3Normalize(dest);

		float dot = XMVectorGetX(XMVector3Dot(start, dest));

		if (dot > 0.99999f)
		{
			return XMQuaternionIdentity();
		}

		if (dot < -0.99999f)
		{
			XMVECTOR axis = XMVector3Cross(XMVectorSet(1, 0, 0, 0), start);
			if (XMVectorGetX(XMVector3LengthSq(axis)) < 0.00001f)
				axis = XMVector3Cross(XMVectorSet(0, 1, 0, 0), start);

			axis = XMVector3Normalize(axis);
			return XMQuaternionRotationAxis(axis, XM_PI);
		}

		XMVECTOR axis = XMVector3Cross(start, dest);
		XMVECTOR q = XMVectorSetW(axis, 1.0f + dot);
		return XMQuaternionNormalize(q);
	}

	inline XMVECTOR GetRotationFromTwoVectors(XMVECTOR srcDir, XMVECTOR srcUp, XMVECTOR dstDir, XMVECTOR dstUp)
	{
		XMVECTOR qSwing = GetRotationBetweenVectors(srcDir, dstDir);

		if (XMVectorGetX(XMVector3LengthSq(srcUp)) < 0.001f || XMVectorGetX(XMVector3LengthSq(dstUp)) < 0.001f)
		{
			return qSwing;
		}

		XMVECTOR swungUp = XMVector3Rotate(srcUp, qSwing);

		XMVECTOR axis = XMVector3Normalize(dstDir);
		XMVECTOR projSwungUp = XMVector3Normalize(swungUp - axis * XMVector3Dot(swungUp, axis));
		XMVECTOR projDstUp = XMVector3Normalize(dstUp - axis * XMVector3Dot(dstUp, axis));

		XMVECTOR qTwist = GetRotationBetweenVectors(projSwungUp, projDstUp);

		return XMQuaternionMultiply(qSwing, qTwist);
	}

}

namespace Matrix4x4
{
	inline XMFLOAT4X4 Identity()
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixIdentity());
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Zero()
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixSet(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Multiply(XMFLOAT4X4& xmmtx4x4Matrix1, XMFLOAT4X4& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixMultiply(XMLoadFloat4x4(&xmmtx4x4Matrix1), XMLoadFloat4x4(&xmmtx4x4Matrix2)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Multiply(XMFLOAT4X4& xmmtx4x4Matrix1, XMMATRIX& xmmtxMatrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMLoadFloat4x4(&xmmtx4x4Matrix1) * xmmtxMatrix2);
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Multiply(XMMATRIX& xmmtxMatrix1, XMFLOAT4X4& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, xmmtxMatrix1 * XMLoadFloat4x4(&xmmtx4x4Matrix2));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Add(XMFLOAT4X4& xmmtx4x4Matrix1, XMFLOAT4X4& xmmtx4x4Matrix2)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMLoadFloat4x4(&xmmtx4x4Matrix1) + XMLoadFloat4x4(&xmmtx4x4Matrix2));
		return(xmf4x4Result);
	}

	inline XMFLOAT4 QuaternionIdentity()
	{
		XMFLOAT4 q;
		XMStoreFloat4(&q, XMQuaternionIdentity());
		return q;
	}

	inline XMFLOAT4 QuaternionFromAxisAngle(const XMFLOAT3& axis, float angleDeg)
	{
		XMFLOAT4 q;
		XMStoreFloat4(&q, XMQuaternionRotationAxis(XMLoadFloat3(&axis), XMConvertToRadians(angleDeg)));
		return q;
	}

	inline XMFLOAT4 QuaternionFromEuler(float pitch, float yaw, float roll)
	{
		XMFLOAT4 q;
		XMStoreFloat4(&q, XMQuaternionRotationRollPitchYaw(
			XMConvertToRadians(pitch),
			XMConvertToRadians(yaw),
			XMConvertToRadians(roll)));
		return q;
	}

	inline XMFLOAT4 QuaternionFromEulerRad(float pitchRad, float yawRad, float rollRad)
	{
		XMFLOAT4 q;
		XMStoreFloat4(&q, XMQuaternionRotationRollPitchYaw(pitchRad, yawRad, rollRad));
		return q;
	}


	inline XMFLOAT4X4 MatrixFromQuaternion(const XMFLOAT4& quat)
	{
		XMFLOAT4X4 m;
		XMStoreFloat4x4(&m, XMMatrixRotationQuaternion(XMLoadFloat4(&quat)));
		return m;
	}

	inline XMFLOAT4 QuaternionSlerp(const XMFLOAT4& q1, const XMFLOAT4& q2, float t)
	{
		XMFLOAT4 q;
		XMStoreFloat4(&q, XMQuaternionSlerp(XMLoadFloat4(&q1), XMLoadFloat4(&q2), t));
		return q;
	}

	inline XMFLOAT4 QuaternionNormalize(const XMFLOAT4& qIn)
	{
		XMFLOAT4 q;
		XMStoreFloat4(&q, XMQuaternionNormalize(XMLoadFloat4(&qIn)));
		return q;
	}

	inline XMFLOAT4 QuaternionMultiply(const XMFLOAT4& q1, const XMFLOAT4& q2)
	{
		XMFLOAT4 q;
		XMStoreFloat4(&q, XMQuaternionMultiply(XMLoadFloat4(&q1), XMLoadFloat4(&q2)));
		return q;
	}

	inline XMFLOAT4X4 RotateAxis(XMFLOAT3& xmf3Axis, float fAngle)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixRotationAxis(XMLoadFloat3(&xmf3Axis), XMConvertToRadians(fAngle)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Rotate(float x, float y, float z)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixRotationRollPitchYaw(XMConvertToRadians(x), XMConvertToRadians(y), XMConvertToRadians(z)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 AffineTransformation(XMFLOAT3& xmf3Scaling, XMFLOAT3& xmf3RotateOrigin, XMFLOAT3& xmf3Rotation, XMFLOAT3& xmf3Translation)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixAffineTransformation(XMLoadFloat3(&xmf3Scaling), XMLoadFloat3(&xmf3RotateOrigin), XMQuaternionRotationRollPitchYaw(XMConvertToRadians(xmf3Rotation.x), XMConvertToRadians(xmf3Rotation.y), XMConvertToRadians(xmf3Rotation.z)), XMLoadFloat3(&xmf3Translation)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Interpolate(XMFLOAT4X4& xmf4x4Matrix1, XMFLOAT4X4& xmf4x4Matrix2, float t)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMVECTOR S0, R0, T0, S1, R1, T1;
		XMMatrixDecompose(&S0, &R0, &T0, XMLoadFloat4x4(&xmf4x4Matrix1));
		XMMatrixDecompose(&S1, &R1, &T1, XMLoadFloat4x4(&xmf4x4Matrix2));
		XMVECTOR S = XMVectorLerp(S0, S1, t);
		XMVECTOR T = XMVectorLerp(T0, T1, t);
		XMVECTOR R = XMQuaternionSlerp(R0, R1, t);
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixAffineTransformation(S, XMVectorZero(), R, T));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Inverse(const XMFLOAT4X4& xmmtx4x4Matrix)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixInverse(NULL, XMLoadFloat4x4(&xmmtx4x4Matrix)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 Transpose(const XMFLOAT4X4& xmmtx4x4Matrix)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixTranspose(XMLoadFloat4x4(&xmmtx4x4Matrix)));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 PerspectiveFovLH(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, NearZ, FarZ));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 PerspectiveFovLH_Reverse_Z(float FovAngleY, float AspectRatio, float NearZ, float FarZ)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixPerspectiveFovLH(FovAngleY, AspectRatio, FarZ, NearZ));
		return(xmf4x4Result);
	}

	inline XMFLOAT4X4 LookAtLH(XMFLOAT3& xmf3EyePosition, XMFLOAT3& xmf3LookAtPosition, XMFLOAT3& xmf3UpDirection)
	{
		XMFLOAT4X4 xmf4x4Result;
		XMStoreFloat4x4(&xmf4x4Result, XMMatrixLookAtLH(XMLoadFloat3(&xmf3EyePosition), XMLoadFloat3(&xmf3LookAtPosition), XMLoadFloat3(&xmf3UpDirection)));
		return(xmf4x4Result);
	}
}

inline void XMQuaternionToRollPitchYaw(FXMVECTOR q, float* pRoll, float* pPitch, float* pYaw)
{
	float ysqr = XMVectorGetY(q) * XMVectorGetY(q);

	float t0 = 2.0f * (XMVectorGetW(q) * XMVectorGetZ(q) + XMVectorGetX(q) * XMVectorGetY(q));
	float t1 = 1.0f - 2.0f * (ysqr + XMVectorGetZ(q) * XMVectorGetZ(q));
	float roll = atan2f(t0, t1);

	float t2 = 2.0f * (XMVectorGetW(q) * XMVectorGetX(q) - XMVectorGetY(q) * XMVectorGetZ(q));
	t2 = std::clamp(t2, -1.0f, 1.0f);
	float pitch = asinf(t2);

	float t3 = 2.0f * (XMVectorGetW(q) * XMVectorGetY(q) + XMVectorGetZ(q) * XMVectorGetX(q));
	float t4 = 1.0f - 2.0f * (XMVectorGetX(q) * XMVectorGetX(q) + ysqr);
	float yaw = atan2f(t3, t4);

	if (pRoll)  *pRoll = roll;
	if (pPitch) *pPitch = pitch;
	if (pYaw)   *pYaw = yaw;
}