OVR_SDK_DIR = "e:/carbon/carbon/packages/OculusSDK/"
--OVR_SDK_DIR = "c:/packages/sdks/OculusSDK/"

if not os.isdir(OVR_SDK_DIR) then
	print("OVR_SDK_DIR does not point to a valid directory.\nPlease set in MUST_EDIT_OVR_DIR.lua")
	os.exit(1)
end
