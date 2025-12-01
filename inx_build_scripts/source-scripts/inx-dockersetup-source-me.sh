#!/bin/bash
######################################################################################################
### Script to restart the running script with a specific Dockerimage.
######################################################################################################


check_and_run_docker(){
	#if  [ "$DOCKER_RUNNING" == "true" ]; then
	if [ -f /.dockerenv ]; then
		echo "Already running in Docker continuing to run caller in Docker ..."
		echo "---------------------------------------------------->"
	else
	 echo "$0"
	 DOCKER_IMAGE=$1
	 echo "$DOCKER_IMAGE"
	 echo "$PWD"
	 cd ../
	 echo "$PWD"
	 echo -n "Found Docker image name (${DOCKER_IMAGE}) :"
	 DOCKER_STAGING_DIR="${PWD}/../TARGET_TREES/DOCKER/cachespace"
	 mkdir -p  "${DOCKER_STAGING_DIR}" ||exit
	 pushd "${DOCKER_STAGING_DIR}" || exit
	 echo " CHecking local cache for ${DOCKER_IMAGE} ..."
	 if ${SUDO_COMMAND} docker image inspect "${DOCKER_IMAGE}"  &> /dev/null ; then 
		 echo "Found - Using local docker image"
		 ${SUDO_COMMAND} docker run --user "$(id -u)":"$(id -g)" --rm --privileged -it \
		 -v "$(pwd)/../../../:/inxware"  -w "/inxware/ert-contrib-middleware/inx_build_scripts"\
		 ${DOCKER_IMAGE}\
		 sh -c "echo 'Running $0 in Docker Container...'  pwd  && "$0""
		 exit
	  else 
		echo "Trying to Pull ${DOCKER_IMAGE} from dockerhub"
		echo "Dockerfile path ="${PATH_TO_TARGET_DOCKERFILE}""
		if  [ -f  ${PATH_TO_TARGET_DOCKERFILE} ] ; then
			echo "Checking image exists ..."
			mkdir -p  ${DOCKER_STAGING_DIR} || exit
			${SUDO_COMMAND} docker pull  ${DOCKER_IMAGE} || echo "Could not find  ${DOCKER_IMAGE} in remote repository.  use make publishddockerimage to fix this if you have a Dockerfile"
			${SUDO_COMMAND} docker run --user "$(id -u):$(id -g)" --rm --privileged -it \
			 -v "$(pwd)/../../../:/inxware"  -w "/inxware/ert-contrib-middleware/inx_build_scripts"\
			${DOCKER_IMAGE}\
			sh -c "echo 'Running $0 in Docker Container...' && pwd  && "$0""
			exit
		 fi
	fi
	    popd
	    
	fi
}

