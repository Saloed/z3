package saloed.z3;

import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;

public class NativeLibraryUtil {
    private NativeLibraryUtil() {
    }

    public static void loadLibrary(final String name) {
        final String fullLibraryName = System.mapLibraryName(name);
        final Path libraryFile = extractLibraryFromJarLib(fullLibraryName);
        if (libraryFile == null) {
            throw new IllegalStateException("Unable to extract native library " + fullLibraryName + " from jar");
        }
        System.load(libraryFile.toAbsolutePath().toString());
    }

    private static Path extractLibraryFromJarLib(final String libName) {
        Path libFile = null;
        try (final InputStream inputStream = NativeLibraryUtil.class
                .getClassLoader().getResourceAsStream("lib/" + libName)) {
            libFile = Files.createTempFile("z3-native-", libName);
            Files.copy(inputStream, libFile, StandardCopyOption.REPLACE_EXISTING);
            libFile.toFile().deleteOnExit();
        } catch (IOException e) {
            e.printStackTrace();
        }
        return libFile;
    }

}
