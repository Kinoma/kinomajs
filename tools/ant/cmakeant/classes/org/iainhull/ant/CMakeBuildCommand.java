package org.iainhull.ant;

import java.util.ArrayList;
import java.util.List;

public class CMakeBuildCommand extends BuildCommand {
	private final boolean canSkipCmakeStep; 
	
	
	public CMakeBuildCommand(GeneratorRule generator, CacheVariables vars) {
		super(generator, vars);
		canSkipCmakeStep = ! isVisualStudio(vars.getVariable(Variable.CMAKE_GENERATOR).getValue());
		assert isSupported(vars);
	}

	private static boolean isVisualStudio(String value) {
		return value.startsWith("Visual Studio");
	}

	@Override
	protected List<String> buildCommand() {
		List<String> ret = new ArrayList<String>();
		ret.add(CmakeBuilder.CMAKE_COMMAND);
		ret.add("--build");
		ret.add(generator.getBindir().toString());
		
		if (generator.getTarget() != null) {
			ret.add("--target");
			ret.add(generator.getTarget());
		}
		
		if (generator.getBuildtype() != null) {
			ret.add("--config");
			ret.add(generator.getBuildtype().toString());
		}
		
		if (generator.isCleanfirst()) {
			ret.add("--clean-first");
		}
		
		List<String> buildArgs = generator.getBuildargsAsList();
		if (!buildArgs.isEmpty()) {
			ret.add("--");
			ret.addAll(buildArgs);
		}
		
		return ret;
	}

	@Override
	protected boolean canBuild() {
		return true;
	}

	@Override
	protected boolean canSkipCmakeStep() {
		return canSkipCmakeStep;
	}
	
	/**
	 * CMake 2.8 and above support the --build command line option to invoke the
	 * appropriate build.
	 * 
	 * @param vars
	 * @return
	 */
	public static boolean isSupported(CacheVariables vars) {
		try {
			int major = vars.getIntValue(Variable.CMAKE_MAJOR_VERSION, 0);
			int minor = vars.getIntValue(Variable.CMAKE_MINOR_VERSION, 0);

			return major > 2 || (major == 2 && minor >= 8);
		} catch (NumberFormatException e) {
			return false;
		}
	}
}
