#ifndef _Vfs_Dataset_ExtList_h_
#define _Vfs_Dataset_ExtList_h_

#if 0
#define NODE_LIST \
	DATASET_ITEM(VfsValue,			env,			METAKIND_PKG_ENV,							CATEGORY_ECS,"Environment") \
	DATASET_ITEM(VfsValue,			ctx,			METAKIND_CONTEXT,							CATEGORY_ECS,"Context") \
	DATASET_ITEM(VfsValue,			dbref,			METAKIND_DB_REF,							CATEGORY_ECS,"Database Reference") \
	DATASET_ITEM(VfsValue,			comment,		METAKIND_COMMENT,							CATEGORY_ECS,"Comment") \
	DATASET_ITEM(VfsValue,			space,			METAKIND_ECS_SPACE,							CATEGORY_ECS,"Space") \

#define BASE_EXT_LIST \
	DATASET_ITEM(Entity,			entity,			METAKIND_ECS_ENTITY,						CATEGORY_ECS, "Entity") \
	DATASET_ITEM(Component,			component,		METAKIND_ECS_COMPONENT_UNDEFINED,			CATEGORY_ECS, "Undefined Component") \

// TODO rename many of these to have same name class, field-name, kind (e.g. lyric_struct, METAKIND_ECS_COMPONENT_SONG_IDEA)

// see SRC_TXT_HEADER_ENABLE

#define COMPONENT_LIST \
	DATASET_ITEM(ProjectWizardView,	prjwiz,			METAKIND_ECS_COMPONENT_PROJECT_WIZARD,		CATEGORY_ASSET, "Code Project Wizard") \
	DATASET_ITEM(VfsProgram,		vfs_prog,		METAKIND_ECS_COMPONENT_VFS_PROGRAM,			CATEGORY_DISPOSABLE, "Vfs: Program") \
	/*TODO SrcTxtHeader: non-component kind can't be added, but it's ok*/ \
	DATASET_ITEM(SrcTxtHeader,		src,			METAKIND_DATABASE_SOURCE,					CATEGORY_DISPOSABLE, "Source Database") \
	DATASET_ITEM(AiChatComponent,	ai_chat,		METAKIND_ECS_COMPONENT_AI_CHAT,				CATEGORY_DISPOSABLE, "AI: Chat") \
	DATASET_ITEM(AiCompletionComponent,	ai_compl,	METAKIND_ECS_COMPONENT_AI_COMPLETION,		CATEGORY_DISPOSABLE, "AI: Completion") \
	DATASET_ITEM(AiStageExample,	ai_stage_ex,	METAKIND_ECS_COMPONENT_AI_STAGE_EXAMPLE,	CATEGORY_DISPOSABLE, "AI: Stage") \
	DATASET_ITEM(ChainThread,		ai_chain_thrd,	METAKIND_ECS_COMPONENT_AI_CHAIN,			CATEGORY_DISPOSABLE, "AI: Chain Thread") \
	DATASET_ITEM(Agent,				ai_agent,		METAKIND_ECS_COMPONENT_AI_AGENT,			CATEGORY_DISPOSABLE, "AI: Agent") \
	\
	DATASET_ITEM(Owner,					owner,				METAKIND_ECS_COMPONENT_OWNER,						CATEGORY_PRIVATE, "Owner") /* TODO rename to human? */ \
	DATASET_ITEM(Notepad,				notepad,			METAKIND_ECS_COMPONENT_NOTEPAD,						CATEGORY_PRIVATE, "Notepad") \
	DATASET_ITEM(Biography,				biography,			METAKIND_ECS_COMPONENT_BIOGRAPHY,					CATEGORY_PRIVATE, "Biography") \
	DATASET_ITEM(BiographyPlatform,		analysis,			METAKIND_ECS_COMPONENT_BIOGRAPHY_ANALYSIS,			CATEGORY_PRIVATE, "Biography Platform") \
	DATASET_ITEM(BiographyPerspectives,	snap,				METAKIND_ECS_COMPONENT_BIOGRAPHY_PERSPECTIVES,		CATEGORY_PRIVATE, "Biography Perspectives") \
	DATASET_ITEM(Profile,				profile,			METAKIND_ECS_COMPONENT_PROFILE,						CATEGORY_PUBLIC, "Profile") \
	DATASET_ITEM(Release,				release,			METAKIND_ECS_COMPONENT_RELEASE,						CATEGORY_PUBLIC, "Release") \
	DATASET_ITEM(PerspectiveComponent,	perspective,		METAKIND_ECS_COMPONENT_PERSPECTIVE,					CATEGORY_PUBLIC, "Perspective") \
	DATASET_ITEM(Artist,				artist,				METAKIND_ECS_COMPONENT_ARTIST,						CATEGORY_PUBLIC, "Artist") \
	DATASET_ITEM(ReleaseBriefing,		rel_brief,			METAKIND_ECS_COMPONENT_RELEASE_BRIEFING,			CATEGORY_PUBLIC, "Release Briefing") \
	DATASET_ITEM(ReleaseCoverImage,		rel_cover_img,		METAKIND_ECS_COMPONENT_RELEASE_COVER_IMAGE,			CATEGORY_PUBLIC, "Release Cover Image") \
	DATASET_ITEM(PlatformManager,		platmgr,			METAKIND_ECS_COMPONENT_PLATFORM_MANAGER,			CATEGORY_PUBLIC, "Platform Manager") \
	DATASET_ITEM(Litigation,			litigation,			METAKIND_ECS_COMPONENT_LITIGATION,					CATEGORY_PUBLIC, "Litigation") \
	DATASET_ITEM(Lawyer,				lawyer,				METAKIND_ECS_COMPONENT_LAWYER,						CATEGORY_PUBLIC, "Lawyer") \
	DATASET_ITEM(Judge,					judge,				METAKIND_ECS_COMPONENT_JUDGE,						CATEGORY_PUBLIC, "Judge") \
	DATASET_ITEM(Lobbying,				lobbying,			METAKIND_ECS_COMPONENT_LOBBYING,					CATEGORY_PUBLIC, "Lobbying") \
	\
	DATASET_ITEM(Provider,			provider,		METAKIND_ECS_COMPONENT_PROVIDER,			CATEGORY_MALE, "Provider") \
	DATASET_ITEM(DecisionMaker,		decision_maker,	METAKIND_ECS_COMPONENT_DECISION_MAKER,		CATEGORY_MALE, "Decision Maker") \
	DATASET_ITEM(Assertions,		assertions	,	METAKIND_ECS_COMPONENT_ASSERTIONS,			CATEGORY_MALE, "Assertions") \
	DATASET_ITEM(Aggressions,		aggressions,	METAKIND_ECS_COMPONENT_AGGRESSIONS,			CATEGORY_MALE, "Aggressions") \
	DATASET_ITEM(Strength,			strength,		METAKIND_ECS_COMPONENT_STRENGTH,			CATEGORY_MALE, "Strength") \
	DATASET_ITEM(Leadership,		leadership,		METAKIND_ECS_COMPONENT_LEADERSHIP,			CATEGORY_MALE, "Leadership") \
	DATASET_ITEM(Justice,			justice,		METAKIND_ECS_COMPONENT_JUSTICE,				CATEGORY_MALE, "Justice") \
	DATASET_ITEM(Sentimentality,	sentimentality,	METAKIND_ECS_COMPONENT_SENTIMENTALITY,		CATEGORY_FEMALE, "Sentimentality") \
	DATASET_ITEM(Addiction,			addiction,		METAKIND_ECS_COMPONENT_ADDICTION,			CATEGORY_FEMALE, "Addiction") \
	DATASET_ITEM(Caretaker,			caretaker,		METAKIND_ECS_COMPONENT_CARETAKER,			CATEGORY_FEMALE, "Caretaker") \
	DATASET_ITEM(SupportSystem,		support_sys,	METAKIND_ECS_COMPONENT_SUPPORT_SYSTEM,		CATEGORY_FEMALE, "Support System") \
	DATASET_ITEM(BeautyStandard,	beauty_std,		METAKIND_ECS_COMPONENT_BEAUTY_STANDARD,		CATEGORY_FEMALE, "Beauty Standard") \
	DATASET_ITEM(Attractiveness,	attractiveness,	METAKIND_ECS_COMPONENT_ATTRACTIVENESS,		CATEGORY_FEMALE, "Attractiveness") \
	DATASET_ITEM(Willingness,		willingness,	METAKIND_ECS_COMPONENT_WILLINGNESS,			CATEGORY_FEMALE, "Willingness") \
	\
	DATASET_ITEM(FactoryComponent,	factory,		METAKIND_ECS_COMPONENT_FACTORY,				CATEGORY_SELLER, "Factory") \
	DATASET_ITEM(ProductComponent,	product,		METAKIND_ECS_COMPONENT_PRODUCT,				CATEGORY_SELLER, "Product") \
	\
	DATASET_ITEM(ConsumerComponent,	consumer,		METAKIND_ECS_COMPONENT_CONSUMER,			CATEGORY_CONSUMER, "Consumer") \
	DATASET_ITEM(LeadData,			lead_data,		METAKIND_ECS_COMPONENT_LEAD_DATA,			CATEGORY_MARKETER, "Lead Data") \
	DATASET_ITEM(LeadDataTemplate,	lead_tmpl,		METAKIND_ECS_COMPONENT_LEAD_TEMPLATE,		CATEGORY_MARKETER, "Lead Template") \
	DATASET_ITEM(LeadDataPublisher,	ld_pub,			METAKIND_ECS_COMPONENT_LEAD_PUBLISHER,		CATEGORY_MARKETER, "Lead Publisher") \
	\
	DATASET_ITEM(CompositionComponent,	composition,	METAKIND_ECS_COMPONENT_COMPOSITION,			CATEGORY_SOUND, "Composition") \
	DATASET_ITEM(LyricalStructure,		lyric_struct,	METAKIND_ECS_COMPONENT_SONG_IDEA,			CATEGORY_SOUND, "Song Idea") \
	DATASET_ITEM(AudioTranscript,		audio_trans,	METAKIND_ECS_COMPONENT_AUDIO_TRANSCRIPT,	CATEGORY_SOUND, "Audio -> Transcript") \
	DATASET_ITEM(Script,				script,			METAKIND_ECS_COMPONENT_SCRIPT,				CATEGORY_TEXT, "Lyrics Draft") /* TODO rename to lyrics_draft */ \
	DATASET_ITEM(Lyrics,				lyrics,			METAKIND_ECS_COMPONENT_LYRICS,				CATEGORY_TEXT, "Lyrics") \
	DATASET_ITEM(Song,					song,			METAKIND_ECS_COMPONENT_SONG,				CATEGORY_TEXT, "Song") \
	DATASET_ITEM(ScriptReasoning,		script_reason,	METAKIND_ECS_COMPONENT_SCRIPT_REASONING,	CATEGORY_TEXT, "Script Reasoning") \
	DATASET_ITEM(ScriptText,			script_text,	METAKIND_ECS_COMPONENT_SCRIPT_TEXT,			CATEGORY_TEXT, "Script Text") \
	DATASET_ITEM(TranscriptProofread,	trans_proof,	METAKIND_ECS_COMPONENT_TRANSCRIPT_PROOFREAD,CATEGORY_TEXT, "Transcript -> Proofread") \
	/*TODO remove DATASET_ITEM(ProofreadStoryline,	proof_story,	METAKIND_ECS_COMPONENT_PROOFREAD_STORYLINE,	CATEGORY_TEXT, "Proofread -> Storyline")*/ \
	/*DATASET_ITEM(StorylineDialog,		storydialog,	METAKIND_ECS_COMPONENT_STORYLINE_DIALOG,	CATEGORY_TEXT, "Storyline -> Dialog")*/ \
	DATASET_ITEM(StorylineConversion,	storyconv,		METAKIND_ECS_COMPONENT_STORYLINE_CONVERSION,CATEGORY_TEXT, "Storyline -> Storyline") \
	DATASET_ITEM(StorylineScript,		storyscript,	METAKIND_ECS_COMPONENT_STORYLINE_SCRIPT,	CATEGORY_TEXT, "Storyline -> Script") \
	DATASET_ITEM(ScriptConversion,		scriptconv,		METAKIND_ECS_COMPONENT_SCRIPT_CONVERSION,	CATEGORY_TEXT, "Script -> Script") \
	DATASET_ITEM(ScriptSpeech,			scriptspeech,	METAKIND_ECS_COMPONENT_SCRIPT_SPEECH,		CATEGORY_TEXT, "Script -> Speech") \
	\
	DATASET_ITEM(ImageLayer,			img_layer,				METAKIND_ECS_COMPONENT_IMG_LAYER,				CATEGORY_PHOTO, "Image layer") \
	DATASET_ITEM(ImageGenLayer,			img_gen_layer,			METAKIND_ECS_COMPONENT_IMG_GEN_LAYER,			CATEGORY_PHOTO, "Generate Image Layer") \
	DATASET_ITEM(AspectFixerLayer,		aspect_fixer,			METAKIND_ECS_COMPONENT_IMG_ASPECT_FIXER_LAYER,	CATEGORY_PHOTO, "Aspect Fix Image Layer") \
	DATASET_ITEM(VideoPromptMaker,		video_prompt_header,	METAKIND_ECS_COMPONENT_VIDEO_PROMPT_MAKER,		CATEGORY_VIDEO, "Video Prompt Maker") \
	DATASET_ITEM(VideoStoryboard,		vid_storyboard,			METAKIND_ECS_COMPONENT_VIDEO_STORYBOARD,		CATEGORY_VIDEO, "Video Storyboard") \
	DATASET_ITEM(VideoSourceFile,		vid_src_file,			METAKIND_ECS_COMPONENT_VIDEO_SOURCE_FILE,		CATEGORY_VIDEO, "Video Source File") \
	DATASET_ITEM(VideoSourceFileRange,	vid_src_file_range,		METAKIND_ECS_COMPONENT_VIDEO_SOURCE_FILE_RANGE,	CATEGORY_VIDEO, "Video Source File Range") \

#define VIRTUALNODE_DATASET_LIST \
	DATASET_ITEM(SrcTextData,		srctxt,			METAKIND_ECS_VIRTUAL_VALUE_SRCTEXT,			CATEGORY_DISPOSABLE, "Source Text") \

#define EXT_LIST \
	BASE_EXT_LIST \
	COMPONENT_LIST \
	VIRTUALNODE_DATASET_LIST \

#define DATASET_LIST \
	NODE_LIST \
	EXT_LIST \

#else

#define NODE_LIST \
	DATASET_ITEM(VfsValue,				env,			"Environment") \
	DATASET_ITEM(VfsValue,				ctx,			"Context") \
	DATASET_ITEM(VfsValue,				dbref,			"Database Reference") \
	DATASET_ITEM(VfsValue,				comment,		"Comment") \
	DATASET_ITEM(VfsValue,				space,			"Space") \

#define BASE_EXT_LIST \
	DATASET_ITEM(Entity,				entity,			"Entity") \
	DATASET_ITEM(Component,				component,		"Undefined Component") \

#define COMPONENT_LIST \
	DATASET_ITEM(SrcTxtHeader,			src,			"Source Database") \
	DATASET_ITEM(Profile,				profile,		"Profile") \
	DATASET_ITEM(Script,				script,			"Lyrics Draft") \
	DATASET_ITEM(LyricalStructure,		lyric_struct,	"Song Idea") \
	DATASET_ITEM(Lyrics,				lyrics,	        "Lyrics") \
	DATASET_ITEM(BiographyPlatform,		analysis,		"Biography Platform") \
	DATASET_ITEM(PlatformManager,		platmgr,		"Platform Manager") \
	DATASET_ITEM(Owner,					owner,			"Owner") /* TODO rename to human? */ \
	DATASET_ITEM(BiographyPerspectives,	snap,			"Biography Perspectives") \
	DATASET_ITEM(Biography,				biography,		"Biography") \
	DATASET_ITEM(Release,				release,		"Release") \
	DATASET_ITEM(Song,					song,			"Song") \
	DATASET_ITEM(LeadData,				lead_data,		"Lead Data") \
	DATASET_ITEM(PerspectiveComponent,	perspective,	"Perspective") \
	DATASET_ITEM(LeadDataTemplate,		lead_tmpl,		"Lead Template") \
	
#define VIRTUALNODE_DATASET_LIST \
	DATASET_ITEM(SrcTextData,			srctxt,			"Source Text") \

#define EXT_LIST \
	BASE_EXT_LIST \
	COMPONENT_LIST \
	VIRTUALNODE_DATASET_LIST \

#define DATASET_LIST \
	NODE_LIST \
	EXT_LIST \

#endif

#endif
