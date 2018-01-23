// Samples.cpp --- Sample Resources
//////////////////////////////////////////////////////////////////////////////
// RisohEditor --- Win32API resource editor
// Copyright (C) 2017-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//////////////////////////////////////////////////////////////////////////////

#include "stdafx.hpp"

//////////////////////////////////////////////////////////////////////////////

static const BYTE abAccel[] = {
0x09, 0x00, 0x4E, 0x00, 0x01, 0x00, 0x00, 0x00, 0x89, 0x00, 0x4F, 0x00, 0x02, 
0x00, 0x00, 0x00, 
};
static const BYTE abDialog[] = {
0xC0, 0x00, 0xC8, 0x80, 0x00, 0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0x00, 
0x00, 0xD7, 0x00, 0x87, 0x00, 0x00, 0x00, 0x00, 0x00, 0x53, 0x00, 0x61, 0x00, 
0x6D, 0x00, 0x70, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x20, 0x00, 0x44, 0x00, 0x69, 
0x00, 0x61, 0x00, 0x6C, 0x00, 0x6F, 0x00, 0x67, 0x00, 0x00, 0x00, 0x09, 0x00, 
0x4D, 0x00, 0x53, 0x00, 0x20, 0x00, 0x53, 0x00, 0x68, 0x00, 0x65, 0x00, 0x6C, 
0x00, 0x6C, 0x00, 0x20, 0x00, 0x44, 0x00, 0x6C, 0x00, 0x67, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x01, 0x00, 0x01, 0x50, 0x00, 0x00, 0x00, 0x00, 0x23, 0x00, 0x73, 
0x00, 0x3C, 0x00, 0x0E, 0x00, 0x01, 0x00, 0x42, 0x00, 0x55, 0x00, 0x54, 0x00, 
0x54, 0x00, 0x4F, 0x00, 0x4E, 0x00, 0x00, 0x00, 0x4F, 0x00, 0x4B, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x50, 0x00, 0x00, 0x00, 0x00, 0x73, 0x00, 
0x73, 0x00, 0x3C, 0x00, 0x0E, 0x00, 0x02, 0x00, 0x42, 0x00, 0x55, 0x00, 0x54, 
0x00, 0x54, 0x00, 0x4F, 0x00, 0x4E, 0x00, 0x00, 0x00, 0x43, 0x00, 0x61, 0x00, 
0x6E, 0x00, 0x63, 0x00, 0x65, 0x00, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 
};
static const BYTE abMenu[] = {
0x00, 0x00, 0x00, 0x00, 0x90, 0x00, 0x26, 0x00, 0x46, 0x00, 0x69, 0x00, 0x6C, 
0x00, 0x65, 0x00, 0x00, 0x00, 0x80, 0x00, 0x01, 0x00, 0x45, 0x00, 0x26, 0x00, 
0x78, 0x00, 0x69, 0x00, 0x74, 0x00, 0x00, 0x00, 
};
static const BYTE abString[] = {
0x00, 0x00, 0x16, 0x00, 0x54, 0x00, 0x68, 0x00, 0x69, 0x00, 0x73, 0x00, 0x20, 
0x00, 0x69, 0x00, 0x73, 0x00, 0x20, 0x00, 0x61, 0x00, 0x20, 0x00, 0x73, 0x00, 
0x61, 0x00, 0x6D, 0x00, 0x70, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x20, 0x00, 0x74, 
0x00, 0x65, 0x00, 0x78, 0x00, 0x74, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
};
static const BYTE abVersion[] = {
0x94, 0x02, 0x34, 0x00, 0x00, 0x00, 0x56, 0x00, 0x53, 0x00, 0x5F, 0x00, 0x56, 
0x00, 0x45, 0x00, 0x52, 0x00, 0x53, 0x00, 0x49, 0x00, 0x4F, 0x00, 0x4E, 0x00, 
0x5F, 0x00, 0x49, 0x00, 0x4E, 0x00, 0x46, 0x00, 0x4F, 0x00, 0x00, 0x00, 0x00, 
0x00, 0xBD, 0x04, 0xEF, 0xFE, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00, 0x04, 0x00, 0x01, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x00, 0xF4, 0x01, 0x00, 0x00, 0x01, 0x00, 0x53, 0x00, 0x74, 0x00, 0x72, 0x00, 
0x69, 0x00, 0x6E, 0x00, 0x67, 0x00, 0x46, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 
0x00, 0x49, 0x00, 0x6E, 0x00, 0x66, 0x00, 0x6F, 0x00, 0x00, 0x00, 0xD0, 0x01, 
0x00, 0x00, 0x01, 0x00, 0x30, 0x00, 0x34, 0x00, 0x30, 0x00, 0x39, 0x00, 0x30, 
0x00, 0x34, 0x00, 0x45, 0x00, 0x34, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x0D, 0x00, 
0x01, 0x00, 0x43, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x70, 0x00, 0x61, 0x00, 0x6E, 
0x00, 0x79, 0x00, 0x4E, 0x00, 0x61, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x59, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x72, 0x00, 0x20, 0x00, 0x43, 
0x00, 0x6F, 0x00, 0x6D, 0x00, 0x70, 0x00, 0x61, 0x00, 0x6E, 0x00, 0x79, 0x00, 
0x00, 0x00, 0x00, 0x00, 0x4A, 0x00, 0x11, 0x00, 0x01, 0x00, 0x46, 0x00, 0x69, 
0x00, 0x6C, 0x00, 0x65, 0x00, 0x44, 0x00, 0x65, 0x00, 0x73, 0x00, 0x63, 0x00, 
0x72, 0x00, 0x69, 0x00, 0x70, 0x00, 0x74, 0x00, 0x69, 0x00, 0x6F, 0x00, 0x6E, 
0x00, 0x00, 0x00, 0x00, 0x00, 0x59, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x72, 0x00, 
0x20, 0x00, 0x41, 0x00, 0x70, 0x00, 0x70, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x63, 
0x00, 0x61, 0x00, 0x74, 0x00, 0x69, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x28, 0x00, 0x04, 0x00, 0x01, 0x00, 0x46, 0x00, 0x69, 0x00, 0x6C, 
0x00, 0x65, 0x00, 0x56, 0x00, 0x65, 0x00, 0x72, 0x00, 0x73, 0x00, 0x69, 0x00, 
0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x00, 0x2E, 0x00, 0x30, 
0x00, 0x00, 0x00, 0xA0, 0x00, 0x3E, 0x00, 0x01, 0x00, 0x4C, 0x00, 0x65, 0x00, 
0x67, 0x00, 0x61, 0x00, 0x6C, 0x00, 0x43, 0x00, 0x6F, 0x00, 0x70, 0x00, 0x79, 
0x00, 0x72, 0x00, 0x69, 0x00, 0x67, 0x00, 0x68, 0x00, 0x74, 0x00, 0x00, 0x00, 
0x43, 0x00, 0x6F, 0x00, 0x70, 0x00, 0x79, 0x00, 0x72, 0x00, 0x69, 0x00, 0x67, 
0x00, 0x68, 0x00, 0x74, 0x00, 0x20, 0x00, 0x28, 0x00, 0x43, 0x00, 0x29, 0x00, 
0x20, 0x00, 0x32, 0x00, 0x30, 0x00, 0x31, 0x00, 0x37, 0x00, 0x20, 0x00, 0x4B, 
0x00, 0x61, 0x00, 0x74, 0x00, 0x61, 0x00, 0x79, 0x00, 0x61, 0x00, 0x6D, 0x00, 
0x61, 0x00, 0x20, 0x00, 0x48, 0x00, 0x69, 0x00, 0x72, 0x00, 0x6F, 0x00, 0x66, 
0x00, 0x75, 0x00, 0x6D, 0x00, 0x69, 0x00, 0x20, 0x00, 0x4D, 0x00, 0x5A, 0x00, 
0x2E, 0x00, 0x20, 0x00, 0x41, 0x00, 0x6C, 0x00, 0x6C, 0x00, 0x20, 0x00, 0x72, 
0x00, 0x69, 0x00, 0x67, 0x00, 0x68, 0x00, 0x74, 0x00, 0x73, 0x00, 0x20, 0x00, 
0x72, 0x00, 0x65, 0x00, 0x73, 0x00, 0x65, 0x00, 0x72, 0x00, 0x76, 0x00, 0x65, 
0x00, 0x64, 0x00, 0x2E, 0x00, 0x00, 0x00, 0x3A, 0x00, 0x0D, 0x00, 0x01, 0x00, 
0x50, 0x00, 0x72, 0x00, 0x6F, 0x00, 0x64, 0x00, 0x75, 0x00, 0x63, 0x00, 0x74, 
0x00, 0x4E, 0x00, 0x61, 0x00, 0x6D, 0x00, 0x65, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x59, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x72, 0x00, 0x20, 0x00, 0x50, 0x00, 0x72, 
0x00, 0x6F, 0x00, 0x64, 0x00, 0x75, 0x00, 0x63, 0x00, 0x74, 0x00, 0x00, 0x00, 
0x00, 0x00, 0x2C, 0x00, 0x04, 0x00, 0x01, 0x00, 0x50, 0x00, 0x72, 0x00, 0x6F, 
0x00, 0x64, 0x00, 0x75, 0x00, 0x63, 0x00, 0x74, 0x00, 0x56, 0x00, 0x65, 0x00, 
0x72, 0x00, 0x73, 0x00, 0x69, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00, 0x30, 
0x00, 0x2E, 0x00, 0x30, 0x00, 0x00, 0x00, 0x44, 0x00, 0x00, 0x00, 0x01, 0x00, 
0x56, 0x00, 0x61, 0x00, 0x72, 0x00, 0x46, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 
0x00, 0x49, 0x00, 0x6E, 0x00, 0x66, 0x00, 0x6F, 0x00, 0x00, 0x00, 0x00, 0x00, 
0x24, 0x00, 0x04, 0x00, 0x00, 0x00, 0x54, 0x00, 0x72, 0x00, 0x61, 0x00, 0x6E, 
0x00, 0x73, 0x00, 0x6C, 0x00, 0x61, 0x00, 0x74, 0x00, 0x69, 0x00, 0x6F, 0x00, 
0x6E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x09, 0x04, 0xE4, 0x04, 
};
static const BYTE abHtml[] = {
0xEF, 0xBB, 0xBF, 0x3C, 0x21, 0x44, 0x4F, 0x43, 0x54, 0x59, 0x50, 0x45, 0x20, 
0x68, 0x74, 0x6D, 0x6C, 0x3E, 0x0D, 0x0A, 0x3C, 0x68, 0x74, 0x6D, 0x6C, 0x20, 
0x6C, 0x61, 0x6E, 0x67, 0x3D, 0x22, 0x65, 0x6E, 0x22, 0x3E, 0x3C, 0x68, 0x65, 
0x61, 0x64, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x6D, 0x65, 0x74, 
0x61, 0x20, 0x68, 0x74, 0x74, 0x70, 0x2D, 0x65, 0x71, 0x75, 0x69, 0x76, 0x3D, 
0x22, 0x43, 0x6F, 0x6E, 0x74, 0x65, 0x6E, 0x74, 0x2D, 0x54, 0x79, 0x70, 0x65, 
0x22, 0x20, 0x63, 0x6F, 0x6E, 0x74, 0x65, 0x6E, 0x74, 0x3D, 0x22, 0x74, 0x65, 
0x78, 0x74, 0x2F, 0x68, 0x74, 0x6D, 0x6C, 0x3B, 0x20, 0x63, 0x68, 0x61, 0x72, 
0x73, 0x65, 0x74, 0x3D, 0x55, 0x54, 0x46, 0x2D, 0x38, 0x22, 0x3E, 0x0D, 0x0A, 
0x20, 0x20, 0x20, 0x20, 0x3C, 0x74, 0x69, 0x74, 0x6C, 0x65, 0x3E, 0x54, 0x68, 
0x69, 0x73, 0x20, 0x69, 0x73, 0x20, 0x61, 0x20, 0x74, 0x69, 0x74, 0x6C, 0x65, 
0x3C, 0x2F, 0x74, 0x69, 0x74, 0x6C, 0x65, 0x3E, 0x0D, 0x0A, 0x3C, 0x2F, 0x68, 
0x65, 0x61, 0x64, 0x3E, 0x0D, 0x0A, 0x3C, 0x62, 0x6F, 0x64, 0x79, 0x3E, 0x0D, 
0x0A, 0x3C, 0x68, 0x31, 0x3E, 0x53, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x3C, 0x2F, 
0x68, 0x31, 0x3E, 0x0D, 0x0A, 0x3C, 0x70, 0x3E, 0x54, 0x68, 0x69, 0x73, 0x20, 
0x69, 0x73, 0x20, 0x61, 0x20, 0x73, 0x61, 0x6D, 0x70, 0x6C, 0x65, 0x3C, 0x2F, 
0x70, 0x3E, 0x0D, 0x0A, 0x3C, 0x2F, 0x62, 0x6F, 0x64, 0x79, 0x3E, 0x3C, 0x2F, 
0x68, 0x74, 0x6D, 0x6C, 0x3E, 0x0D, 0x0A, 
};
static const BYTE abManifest[] = {
0xEF, 0xBB, 0xBF, 0x3C, 0x3F, 0x78, 0x6D, 0x6C, 0x20, 0x76, 0x65, 0x72, 0x73, 
0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x31, 0x2E, 0x30, 0x22, 0x20, 0x65, 0x6E, 0x63, 
0x6F, 0x64, 0x69, 0x6E, 0x67, 0x3D, 0x22, 0x55, 0x54, 0x46, 0x2D, 0x38, 0x22, 
0x20, 0x73, 0x74, 0x61, 0x6E, 0x64, 0x61, 0x6C, 0x6F, 0x6E, 0x65, 0x3D, 0x22, 
0x79, 0x65, 0x73, 0x22, 0x3F, 0x3E, 0x0D, 0x0A, 0x3C, 0x61, 0x73, 0x73, 0x65, 
0x6D, 0x62, 0x6C, 0x79, 0x20, 0x78, 0x6D, 0x6C, 0x6E, 0x73, 0x3D, 0x22, 0x75, 
0x72, 0x6E, 0x3A, 0x73, 0x63, 0x68, 0x65, 0x6D, 0x61, 0x73, 0x2D, 0x6D, 0x69, 
0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x2D, 0x63, 0x6F, 0x6D, 0x3A, 0x61, 
0x73, 0x6D, 0x2E, 0x76, 0x31, 0x22, 0x20, 0x6D, 0x61, 0x6E, 0x69, 0x66, 0x65, 
0x73, 0x74, 0x56, 0x65, 0x72, 0x73, 0x69, 0x6F, 0x6E, 0x3D, 0x22, 0x31, 0x2E, 
0x30, 0x22, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x74, 0x72, 0x75, 0x73, 0x74, 
0x49, 0x6E, 0x66, 0x6F, 0x20, 0x78, 0x6D, 0x6C, 0x6E, 0x73, 0x3D, 0x22, 0x75, 
0x72, 0x6E, 0x3A, 0x73, 0x63, 0x68, 0x65, 0x6D, 0x61, 0x73, 0x2D, 0x6D, 0x69, 
0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x2D, 0x63, 0x6F, 0x6D, 0x3A, 0x61, 
0x73, 0x6D, 0x2E, 0x76, 0x33, 0x22, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 
0x3C, 0x73, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x3E, 0x0D, 0x0A, 0x20, 
0x20, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 
0x65, 0x64, 0x50, 0x72, 0x69, 0x76, 0x69, 0x6C, 0x65, 0x67, 0x65, 0x73, 0x3E, 
0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x72, 0x65, 
0x71, 0x75, 0x65, 0x73, 0x74, 0x65, 0x64, 0x45, 0x78, 0x65, 0x63, 0x75, 0x74, 
0x69, 0x6F, 0x6E, 0x4C, 0x65, 0x76, 0x65, 0x6C, 0x20, 0x6C, 0x65, 0x76, 0x65, 
0x6C, 0x3D, 0x22, 0x61, 0x73, 0x49, 0x6E, 0x76, 0x6F, 0x6B, 0x65, 0x72, 0x22, 
0x20, 0x75, 0x69, 0x41, 0x63, 0x63, 0x65, 0x73, 0x73, 0x3D, 0x22, 0x66, 0x61, 
0x6C, 0x73, 0x65, 0x22, 0x3E, 0x3C, 0x2F, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 
0x74, 0x65, 0x64, 0x45, 0x78, 0x65, 0x63, 0x75, 0x74, 0x69, 0x6F, 0x6E, 0x4C, 
0x65, 0x76, 0x65, 0x6C, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
0x3C, 0x2F, 0x72, 0x65, 0x71, 0x75, 0x65, 0x73, 0x74, 0x65, 0x64, 0x50, 0x72, 
0x69, 0x76, 0x69, 0x6C, 0x65, 0x67, 0x65, 0x73, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 
0x20, 0x20, 0x3C, 0x2F, 0x73, 0x65, 0x63, 0x75, 0x72, 0x69, 0x74, 0x79, 0x3E, 
0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x2F, 0x74, 0x72, 0x75, 0x73, 0x74, 0x49, 0x6E, 
0x66, 0x6F, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x64, 0x65, 0x70, 0x65, 0x6E, 
0x64, 0x65, 0x6E, 0x63, 0x79, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 
0x64, 0x65, 0x70, 0x65, 0x6E, 0x64, 0x65, 0x6E, 0x74, 0x41, 0x73, 0x73, 0x65, 
0x6D, 0x62, 0x6C, 0x79, 0x3E, 0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 
0x3C, 0x61, 0x73, 0x73, 0x65, 0x6D, 0x62, 0x6C, 0x79, 0x49, 0x64, 0x65, 0x6E, 
0x74, 0x69, 0x74, 0x79, 0x20, 0x74, 0x79, 0x70, 0x65, 0x3D, 0x22, 0x77, 0x69, 
0x6E, 0x33, 0x32, 0x22, 0x20, 0x6E, 0x61, 0x6D, 0x65, 0x3D, 0x22, 0x4D, 0x69, 
0x63, 0x72, 0x6F, 0x73, 0x6F, 0x66, 0x74, 0x2E, 0x57, 0x69, 0x6E, 0x64, 0x6F, 
0x77, 0x73, 0x2E, 0x43, 0x6F, 0x6D, 0x6D, 0x6F, 0x6E, 0x2D, 0x43, 0x6F, 0x6E, 
0x74, 0x72, 0x6F, 0x6C, 0x73, 0x22, 0x20, 0x76, 0x65, 0x72, 0x73, 0x69, 0x6F, 
0x6E, 0x3D, 0x22, 0x36, 0x2E, 0x30, 0x2E, 0x30, 0x2E, 0x30, 0x22, 0x20, 0x70, 
0x72, 0x6F, 0x63, 0x65, 0x73, 0x73, 0x6F, 0x72, 0x41, 0x72, 0x63, 0x68, 0x69, 
0x74, 0x65, 0x63, 0x74, 0x75, 0x72, 0x65, 0x3D, 0x22, 0x2A, 0x22, 0x20, 0x70, 
0x75, 0x62, 0x6C, 0x69, 0x63, 0x4B, 0x65, 0x79, 0x54, 0x6F, 0x6B, 0x65, 0x6E, 
0x3D, 0x22, 0x36, 0x35, 0x39, 0x35, 0x62, 0x36, 0x34, 0x31, 0x34, 0x34, 0x63, 
0x63, 0x66, 0x31, 0x64, 0x66, 0x22, 0x20, 0x6C, 0x61, 0x6E, 0x67, 0x75, 0x61, 
0x67, 0x65, 0x3D, 0x22, 0x2A, 0x22, 0x3E, 0x3C, 0x2F, 0x61, 0x73, 0x73, 0x65, 
0x6D, 0x62, 0x6C, 0x79, 0x49, 0x64, 0x65, 0x6E, 0x74, 0x69, 0x74, 0x79, 0x3E, 
0x0D, 0x0A, 0x20, 0x20, 0x20, 0x20, 0x3C, 0x2F, 0x64, 0x65, 0x70, 0x65, 0x6E, 
0x64, 0x65, 0x6E, 0x74, 0x41, 0x73, 0x73, 0x65, 0x6D, 0x62, 0x6C, 0x79, 0x3E, 
0x0D, 0x0A, 0x20, 0x20, 0x3C, 0x2F, 0x64, 0x65, 0x70, 0x65, 0x6E, 0x64, 0x65, 
0x6E, 0x63, 0x79, 0x3E, 0x0D, 0x0A, 0x3C, 0x2F, 0x61, 0x73, 0x73, 0x65, 0x6D, 
0x62, 0x6C, 0x79, 0x3E, 0x0D, 0x0A, 
};

//////////////////////////////////////////////////////////////////////////////

const BYTE *GetAccelSample(DWORD& dwSize)
{
    dwSize = sizeof(abAccel);
    return abAccel;
}

const BYTE *GetDialogSample(DWORD& dwSize)
{
    dwSize = sizeof(abDialog);
    return abDialog;
}

const BYTE *GetMenuSample(DWORD& dwSize)
{
    dwSize = sizeof(abMenu);
    return abMenu;
}

const BYTE *GetStringSample(DWORD& dwSize)
{
    dwSize = sizeof(abString);
    return abString;
}

const BYTE *GetVersionSample(DWORD& dwSize)
{
    dwSize = sizeof(abVersion);
    return abVersion;
}

const BYTE *GetHtmlSample(DWORD& dwSize)
{
    dwSize = sizeof(abHtml);
    return abHtml;
}

const BYTE *GetManifestSample(DWORD& dwSize)
{
    dwSize = sizeof(abManifest);
    return abManifest;
}

//////////////////////////////////////////////////////////////////////////////
