executable('tpm', ['src/main.cpp', 'src/controller.cpp'],
	dependencies : [ libarch, mbus_proto_dep, hw_proto_dep, fs_proto_dep ],
	install : true
)

custom_target('tpm-server',
	command : [bakesvr, '-o', '@OUTPUT@', '@INPUT@'],
	output : 'tpm.bin',
	input : 'tpm.yml',
	install : true,
	install_dir : server
)