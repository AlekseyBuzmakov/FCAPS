RapidJsonFolder=rapidjson
default:application

application:$(RapidJsonFolder) FORCE_MAKE
	make --directory Sofia-PS

$(RapidJsonFolder):rapidjson.patch
	git clone https://github.com/miloyip/rapidjson.git $(RapidJsonFolder)
	cd $(RapidJsonFolder) && git reset --hard c745c953adf68 && git apply ../rapidjson.patch

.PHONY: FORCE_MAKE
